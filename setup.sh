#!/bin/bash

# AEON Arena Weapon Generator - Setup Script for 165.51.78.254
# Hardcoded for your specific AEON server configuration

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration for your server
PROJECT_NAME="aeon-weapon-generator"
PUBLIC_IP=""  # Will be auto-detected
AEON_SERVER_URL="http://165.51.78.254:3030"  # Your main AEON node
DEFAULT_PORT_START=8080
DEFAULT_PORT_END=8090

# Banner
echo -e "${PURPLE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║              AEON WEAPON GENERATOR - AUTO CONFIG             ║"
echo "║              Connecting to: 165.51.78.254:3030              ║"
echo "║                                                              ║"
echo "║  🎯 MBTI-Based Arena Weapon Generation                       ║"
echo "║  ⚔️  Dynamic 3D Models with Textures                         ║"
echo "║  🎮 Auto-registers with your AEON server                    ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}\n"

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "\n${CYAN}[STEP]${NC} $1"
    echo "----------------------------------------"
}

detect_public_ip() {
    log_info "Auto-detecting public IP address..."
    
    # Try multiple services to get public IP
    PUBLIC_IP=""
    
    # Method 1: httpbin.org
    if [ -z "$PUBLIC_IP" ]; then
        PUBLIC_IP=$(curl -s --connect-timeout 5 https://httpbin.org/ip 2>/dev/null | grep -o '"origin":"[^"]*"' | cut -d'"' -f4 | cut -d',' -f1)
    fi
    
    # Method 2: ipify.org
    if [ -z "$PUBLIC_IP" ]; then
        PUBLIC_IP=$(curl -s --connect-timeout 5 https://api.ipify.org 2>/dev/null)
    fi
    
    # Method 3: icanhazip.com
    if [ -z "$PUBLIC_IP" ]; then
        PUBLIC_IP=$(curl -s --connect-timeout 5 https://icanhazip.com 2>/dev/null | tr -d '\n')
    fi
    
    # Method 4: OpenDNS (only if dig is available)
    if [ -z "$PUBLIC_IP" ] && command -v dig &> /dev/null; then
        PUBLIC_IP=$(dig +short myip.opendns.com @resolver1.opendns.com 2>/dev/null)
    fi
    
    # Method 5: ifconfig.me
    if [ -z "$PUBLIC_IP" ]; then
        PUBLIC_IP=$(curl -s --connect-timeout 5 https://ifconfig.me 2>/dev/null)
    fi
    
    # Validate IP address format
    if [[ $PUBLIC_IP =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        log_success "Detected public IP: $PUBLIC_IP"
    else
        log_error "Failed to detect public IP address. Please check your internet connection."
        log_info "You can manually set PUBLIC_IP in the script if needed."
        exit 1
    fi
}

find_available_port() {
    log_info "Finding available port..."
    
    for port in $(seq $DEFAULT_PORT_START $DEFAULT_PORT_END); do
        if ! netstat -tuln 2>/dev/null | grep -q ":$port " && ! ss -tuln 2>/dev/null | grep -q ":$port "; then
            SERVICE_PORT=$port
            log_success "Found available port: $SERVICE_PORT"
            return 0
        fi
    done
    
    log_error "No available ports found in range $DEFAULT_PORT_START-$DEFAULT_PORT_END"
    exit 1
}

create_service_registry() {
    log_info "Creating service registry for $PUBLIC_IP:$SERVICE_PORT..."
    
    API_URL="http://$PUBLIC_IP:$SERVICE_PORT"
    
    cat > service_registry.json << EOF
{
    "service_name": "aeon-weapon-generator",
    "version": "1.0.0",
    "api_url": "$API_URL",
    "public_ip": "$PUBLIC_IP",
    "port": $SERVICE_PORT,
    "aeon_server_url": "$AEON_SERVER_URL",
    "endpoints": {
        "health": "$API_URL/health",
        "generate_weapons": "$API_URL/generate-arena-weapons",
        "mbti_types": "$API_URL/mbti-types",
        "docs": "$API_URL/docs"
    },
    "registered_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "status": "starting"
}
EOF
    
    log_success "Service registry created: service_registry.json"
    log_info "API URL: $API_URL"
    log_info "Will register with: $AEON_SERVER_URL"
}

install_python_deps() {
    log_info "Installing Python dependencies..."
    
    cat > requirements.txt << 'EOF'
torch>=2.0.0
torchvision
fastapi>=0.104.0
uvicorn[standard]>=0.24.0
pydantic>=2.0.0
transformers>=4.35.0
pillow>=10.0.0
numpy>=1.24.0
requests>=2.31.0
aiofiles>=23.0.0
python-multipart>=0.0.6
huggingface-hub>=0.17.0
accelerate>=0.20.0
safetensors>=0.3.0
ipywidgets>=8.0.0
jupyter>=1.0.0
tqdm>=4.65.0
scipy>=1.10.0
matplotlib>=3.7.0
psutil>=5.9.0
EOF

    pip install -r requirements.txt
    log_success "Python dependencies installed"
}

install_shap_e() {
    log_info "Installing Shap-E..."
    
    if [ ! -d "shap-e" ]; then
        git clone https://github.com/openai/shap-e.git
    fi
    
    cd shap-e
    pip install -e .
    cd ..
    
    log_success "Shap-E installed"
}

create_enhanced_weapon_generator() {
    log_info "Creating enhanced weapon generator service..."
    
    cat > weapon_generator.py << 'EOF'
import torch
import json
import uuid
import random
import numpy as np
import time
import os
import psutil
import requests
from fastapi import FastAPI, HTTPException, BackgroundTasks
from pydantic import BaseModel
from typing import List, Dict, Optional
import asyncio
from PIL import Image
from transformers import GPT2LMHeadModel, GPT2Tokenizer
from shap_e.diffusion.sample import sample_latents
from shap_e.diffusion.gaussian_diffusion import diffusion_from_config
from shap_e.models.download import load_model, load_config
from shap_e.util.notebooks import decode_latent_mesh

# Load service configuration
with open('service_registry.json', 'r') as f:
    SERVICE_CONFIG = json.load(f)

app = FastAPI(
    title="AEON Arena Weapon Generator - MBTI Edition", 
    version="1.0.0",
    description="Dynamic 3D weapon generation based on MBTI personality types"
)

# Request/Response Models
class ArenaRequest(BaseModel):
    player1_mbti: str
    player2_mbti: str

class WeaponData(BaseModel):
    WeaponName: str
    description: str
    fileLocation: str
    textureLocation: Optional[str] = None
    materialLocation: Optional[str] = None
    damage: int
    speed: float
    rarity: str
    colors: Optional[Dict[str, str]] = None

class ArenaResponse(BaseModel):
    scenario: str
    weapons: List[WeaponData]
    arena_id: str
    arena_folder: str

class ServiceStatus(BaseModel):
    status: str
    api_url: str
    public_ip: str
    port: int
    device: str
    memory_usage: float
    gpu_memory: Optional[float] = None
    uptime_seconds: float
    total_arenas_generated: int

class WeaponGenerator:
    def __init__(self):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.start_time = time.time()
        self.arenas_generated = 0
        
        print(f"🔥 Using device: {self.device}")
        
        # Load text generation model
        print("📚 Loading GPT-2 model...")
        self.tokenizer = GPT2Tokenizer.from_pretrained("gpt2")
        self.text_model = GPT2LMHeadModel.from_pretrained("gpt2").to(self.device)
        self.tokenizer.pad_token = self.tokenizer.eos_token
        
        # Load Shap-E models
        print("🎨 Loading Shap-E models...")
        self.xm = load_model('transmitter', device=self.device)
        self.shape_model = load_model('text300M', device=self.device)
        self.diffusion = diffusion_from_config(load_config('diffusion'))
        
        # MBTI personality mappings (same as before)
        self.mbti_profiles = {
            "INTJ": {
                "name": "The Architect",
                "traits": ["strategic", "independent", "technological"],
                "colors": ["#2C3E50", "#34495E", "#7F8C8D"],
                "materials": ["steel", "carbon_fiber", "obsidian"],
                "weapon_style": "precise"
            },
            "INTP": {
                "name": "The Thinker", 
                "traits": ["logical", "innovative", "chaotic"],
                "colors": ["#8E44AD", "#9B59B6", "#BDC3C7"],
                "materials": ["crystal", "energy", "unknown"],
                "weapon_style": "experimental"
            },
            "ENTJ": {
                "name": "The Commander",
                "traits": ["aggressive", "strategic", "dominant"],
                "colors": ["#C0392B", "#E74C3C", "#D35400"],
                "materials": ["metal", "volcanic_rock", "steel"],
                "weapon_style": "commanding"
            },
            "ENTP": {
                "name": "The Debater",
                "traits": ["chaotic", "innovative", "energetic"],
                "colors": ["#F39C12", "#E67E22", "#F1C40F"],
                "materials": ["shifting", "plasma", "energy"],
                "weapon_style": "unpredictable"
            },
            "INFJ": {
                "name": "The Advocate",
                "traits": ["magical", "protective", "mystical"],
                "colors": ["#6C5CE7", "#A29BFE", "#74B9FF"],
                "materials": ["ethereal", "crystal", "energy"],
                "weapon_style": "enlightened"
            },
            "INFP": {
                "name": "The Mediator",
                "traits": ["creative", "gentle", "artistic"],
                "colors": ["#FD79A8", "#FDCB6E", "#E17055"],
                "materials": ["wood", "silk", "pearl"],
                "weapon_style": "harmonious"
            },
            "ENFJ": {
                "name": "The Protagonist",
                "traits": ["inspiring", "protective", "radiant"],
                "colors": ["#00B894", "#00CEC9", "#55EFC4"],
                "materials": ["gold", "jade", "light"],
                "weapon_style": "inspiring"
            },
            "ENFP": {
                "name": "The Campaigner",
                "traits": ["enthusiastic", "colorful", "dynamic"],
                "colors": ["#FF6B6B", "#4ECDC4", "#45B7D1"],
                "materials": ["shifting", "rainbow", "energy"],
                "weapon_style": "spirited"
            },
            "ISTJ": {
                "name": "The Logistician",
                "traits": ["reliable", "practical", "sturdy"],
                "colors": ["#5D4E75", "#B2B2B2", "#8B4513"],
                "materials": ["steel", "stone", "iron"],
                "weapon_style": "dependable"
            },
            "ISFJ": {
                "name": "The Protector",
                "traits": ["defensive", "caring", "gentle"],
                "colors": ["#87CEEB", "#DDA0DD", "#F0E68C"],
                "materials": ["silver", "moonstone", "silk"],
                "weapon_style": "protective"
            },
            "ESTJ": {
                "name": "The Executive",
                "traits": ["commanding", "efficient", "strong"],
                "colors": ["#B22222", "#CD853F", "#2F4F4F"],
                "materials": ["steel", "brass", "titanium"],
                "weapon_style": "authoritative"
            },
            "ESFJ": {
                "name": "The Consul",
                "traits": ["harmonious", "supportive", "warm"],
                "colors": ["#FFB6C1", "#98FB98", "#F0E68C"],
                "materials": ["gold", "coral", "pearl"],
                "weapon_style": "nurturing"
            },
            "ISTP": {
                "name": "The Virtuoso",
                "traits": ["practical", "mechanical", "precise"],
                "colors": ["#708090", "#36454F", "#C0C0C0"],
                "materials": ["steel", "carbon_fiber", "titanium"],
                "weapon_style": "crafted"
            },
            "ISFP": {
                "name": "The Adventurer",
                "traits": ["artistic", "flexible", "gentle"],
                "colors": ["#DA70D6", "#20B2AA", "#F4A460"],
                "materials": ["wood", "leather", "feather"],
                "weapon_style": "artistic"
            },
            "ESTP": {
                "name": "The Entrepreneur", 
                "traits": ["bold", "energetic", "action"],
                "colors": ["#FF4500", "#32CD32", "#FFD700"],
                "materials": ["metal", "fire", "lightning"],
                "weapon_style": "dynamic"
            },
            "ESFP": {
                "name": "The Entertainer",
                "traits": ["playful", "colorful", "spontaneous"],
                "colors": ["#FF69B4", "#00FA9A", "#FF6347"],
                "materials": ["rainbow", "crystal", "silk"],
                "weapon_style": "joyful"
            }
        }
        
        self.weapon_types = [
            "sword", "hammer", "staff", "bow", "dagger", "axe", "spear", "whip",
            "frying pan", "umbrella", "coffee mug", "rubber duck", "pencil", 
            "toothbrush", "calculator", "pizza slice", "banana", "smartphone"
        ]
        
        print("✅ All models loaded successfully!")
        
        # Register with AEON server on startup
        self.register_with_aeon_server()

    def register_with_aeon_server(self):
        """Register this service with the AEON main server"""
        try:
            registration_data = {
                "service_name": SERVICE_CONFIG["service_name"],
                "api_url": SERVICE_CONFIG["api_url"],
                "public_ip": SERVICE_CONFIG["public_ip"],
                "port": SERVICE_CONFIG["port"],
                "endpoints": SERVICE_CONFIG["endpoints"],
                "status": "healthy",
                "device": str(self.device)
            }
            
            aeon_url = SERVICE_CONFIG["aeon_server_url"]
            response = requests.post(
                f"{aeon_url}/api/weapon-services/register",
                json=registration_data,
                timeout=10
            )
            
            if response.status_code == 200:
                print(f"✅ Successfully registered with AEON server: {aeon_url}")
            else:
                print(f"⚠️ AEON server registration failed: {response.status_code}")
                
        except Exception as e:
            print(f"⚠️ Could not register with AEON server: {e}")

    def get_system_stats(self) -> Dict:
        """Get current system statistics"""
        memory = psutil.virtual_memory()
        gpu_memory = None
        
        if torch.cuda.is_available():
            gpu_memory = torch.cuda.memory_allocated() / 1024**3  # GB
        
        return {
            "memory_usage": memory.percent,
            "gpu_memory": gpu_memory,
            "uptime_seconds": time.time() - self.start_time,
            "total_arenas_generated": self.arenas_generated
        }

    def generate_arena_id(self) -> str:
        timestamp = int(time.time())
        random_id = uuid.uuid4().hex[:8]
        return f"arena_{timestamp}_{random_id}"

    def create_arena_folder(self, arena_id: str) -> str:
        arena_folder = f"generated_weapons/{arena_id}"
        os.makedirs(arena_folder, exist_ok=True)
        return arena_folder

    def generate_scenario_and_weapons(self, player1_mbti: str, player2_mbti: str) -> Dict:
        profile1 = self.mbti_profiles.get(player1_mbti.upper(), self.mbti_profiles["ISFJ"])
        profile2 = self.mbti_profiles.get(player2_mbti.upper(), self.mbti_profiles["ESTP"])
        
        scenario = self._generate_mbti_scenario(profile1, profile2, player1_mbti, player2_mbti)
        weapons = self._generate_mbti_weapons(profile1, profile2)
        
        return {"scenario": scenario, "weapons": weapons}

    def _generate_mbti_scenario(self, profile1: Dict, profile2: Dict, mbti1: str, mbti2: str) -> str:
        scenarios = [
            f"In the arena, {profile1['name']} ({mbti1}) faces {profile2['name']} ({mbti2}) - a clash of {profile1['weapon_style']} vs {profile2['weapon_style']} combat styles!",
            f"The mystical battleground awakens as {mbti1}'s {profile1['weapon_style']} approach meets {mbti2}'s {profile2['weapon_style']} strategy!",
            f"Two personalities collide: The {profile1['weapon_style']} {profile1['name']} against the {profile2['weapon_style']} {profile2['name']}!",
            f"Ancient forces stir as {mbti1} and {mbti2} enter the arena, their contrasting energies creating unpredictable magical weapons!",
            f"The arena responds to personality clash - {profile1['name']} versus {profile2['name']} in an epic battle of contrasting souls!"
        ]
        return random.choice(scenarios)

    def _generate_mbti_weapons(self, profile1: Dict, profile2: Dict) -> List[Dict]:
        weapons = []
        all_traits = profile1["traits"] + profile2["traits"]
        all_colors = profile1["colors"] + profile2["colors"]
        all_materials = profile1["materials"] + profile2["materials"]
        
        for i in range(4):
            weapon_type = random.choice(self.weapon_types)
            trait1 = random.choice(all_traits)
            trait2 = random.choice(all_traits) if len(all_traits) > 1 else ""
            
            weapon_name = f"{trait1.title()} {weapon_type.title()}"
            if trait2 and trait2 != trait1:
                weapon_name = f"{trait1.title()} {trait2.title()} {weapon_type.title()}"
            
            primary_color = random.choice(all_colors)
            material = random.choice(all_materials)
            
            description = f"a {trait1} {weapon_type} made of {material}"
            if trait2 and trait2 != trait1:
                description = f"a {trait1} {weapon_type} made of {material} with {trait2} details"
            
            if "magical" in all_traits or "mystical" in all_traits:
                description += " with ethereal energy and mystical aura"
            elif "technological" in all_traits:
                description += " with advanced tech and glowing components"
            elif "chaotic" in all_traits:
                description += " with unpredictable patterns and shifting forms"
            elif "artistic" in all_traits:
                description += " with beautiful engravings and elegant design"
            
            damage = random.randint(60, 130)
            speed = round(random.uniform(0.5, 2.5), 1)
            rarities = ["common", "rare", "epic", "legendary"]
            rarity = random.choice(rarities)
            
            secondary_color = self._generate_secondary_color(primary_color)
            
            weapons.append({
                "name": weapon_name,
                "description": description,
                "damage": damage,
                "speed": speed,
                "rarity": rarity,
                "material": material,
                "colors": {"primary": primary_color, "secondary": secondary_color}
            })
        
        return weapons

    def _generate_secondary_color(self, primary_hex: str) -> str:
        primary_rgb = tuple(int(primary_hex[i:i+2], 16) for i in (1, 3, 5))
        secondary_rgb = tuple(255 - c for c in primary_rgb)
        return f"#{secondary_rgb[0]:02x}{secondary_rgb[1]:02x}{secondary_rgb[2]:02x}"

    async def generate_3d_weapons(self, weapons_data: List[Dict], arena_folder: str) -> List[WeaponData]:
        generated_weapons = []
        
        for i, weapon in enumerate(weapons_data):
            try:
                print(f"⚔️  Generating 3D model for: {weapon['name']}")
                
                mesh_result = await self._generate_weapon_mesh(
                    weapon['description'], arena_folder, i, weapon['colors']
                )
                
                material_path = await self._generate_material_file(
                    weapon, arena_folder, i
                )
                
                generated_weapons.append(WeaponData(
                    WeaponName=weapon['name'],
                    description=weapon['description'],
                    fileLocation=mesh_result['obj_path'],
                    textureLocation=mesh_result.get('texture_path'),
                    materialLocation=material_path,
                    damage=weapon['damage'],
                    speed=weapon['speed'],
                    rarity=weapon['rarity'],
                    colors=weapon['colors']
                ))
                print(f"✅ Generated: {weapon['name']}")
                
            except Exception as e:
                print(f"❌ Failed to generate weapon {weapon['name']}: {e}")
                continue
                
        self.arenas_generated += 1
        return generated_weapons

    async def _generate_weapon_mesh(self, description: str, arena_folder: str, weapon_index: int, colors: Dict[str, str]) -> Dict[str, str]:
        latents = sample_latents(
            batch_size=1,
            model=self.shape_model,
            diffusion=self.diffusion,
            guidance_scale=15.0,
            model_kwargs=dict(texts=[description]),
            progress=False,
            clip_denoised=True,
            use_fp16=True,
            use_karras=True,
            karras_steps=64,
            sigma_min=1e-3,
            sigma_max=160,
            s_churn=0,
        )
        
        mesh_obj = decode_latent_mesh(self.xm, latents[0])
        mesh = mesh_obj.tri_mesh()
        
        file_id = f"weapon_{weapon_index}_{uuid.uuid4().hex[:8]}"
        obj_path = f"{arena_folder}/{file_id}.obj"
        texture_path = f"{arena_folder}/{file_id}_texture.png"
        
        self._write_obj_with_materials(mesh, obj_path, file_id, colors)
        
        texture_created = await self._generate_procedural_texture(colors, texture_path, description)
        
        result = {'obj_path': obj_path}
        if texture_created:
            result['texture_path'] = texture_path
            
        return result

    def _write_obj_with_materials(self, mesh, obj_path: str, file_id: str, colors: Dict[str, str]):
        with open(obj_path, 'w') as f:
            f.write(f"mtllib {file_id}.mtl\n")
            f.write("usemtl weapon_material\n")
            
            for vertex in mesh.verts:
                f.write(f"v {vertex[0]} {vertex[1]} {vertex[2]}\n")
            
            for i, vertex in enumerate(mesh.verts):
                u = (vertex[0] + 1) / 2
                v = (vertex[1] + 1) / 2
                f.write(f"vt {u} {v}\n")
            
            for face in mesh.faces:
                f.write(f"f {face[0]+1}/{face[0]+1} {face[1]+1}/{face[1]+1} {face[2]+1}/{face[2]+1}\n")

    async def _generate_material_file(self, weapon: Dict, arena_folder: str, weapon_index: int) -> str:
        file_id = f"weapon_{weapon_index}_{uuid.uuid4().hex[:8]}"
        mtl_path = f"{arena_folder}/{file_id}.mtl"
        
        primary_rgb = [int(weapon['colors']['primary'][i:i+2], 16)/255.0 for i in (1, 3, 5)]
        secondary_rgb = [int(weapon['colors']['secondary'][i:i+2], 16)/255.0 for i in (1, 3, 5)]
        
        with open(mtl_path, 'w') as f:
            f.write("# AEON Generated Weapon Material\n")
            f.write("newmtl weapon_material\n")
            f.write(f"Ka {primary_rgb[0]} {primary_rgb[1]} {primary_rgb[2]}\n")
            f.write(f"Kd {primary_rgb[0]} {primary_rgb[1]} {primary_rgb[2]}\n")
            f.write(f"Ks {secondary_rgb[0]} {secondary_rgb[1]} {secondary_rgb[2]}\n")
            
            if weapon['material'] in ['metal', 'steel', 'titanium']:
                f.write("Ns 96.0\n")
                f.write("Ni 1.5\n")
            elif weapon['material'] in ['crystal', 'energy', 'ethereal']:
                f.write("Ns 200.0\n")
                f.write("Ni 1.8\n")
                f.write("d 0.8\n")
            else:
                f.write("Ns 50.0\n")
                f.write("Ni 1.0\n")
            
            texture_file = f"{file_id}_texture.png"
            f.write(f"map_Kd {texture_file}\n")
        
        return mtl_path

    async def _generate_procedural_texture(self, colors: Dict[str, str], texture_path: str, description: str) -> bool:
        try:
            size = 256
            image = Image.new('RGB', (size, size))
            pixels = image.load()
            
            primary_rgb = tuple(int(colors['primary'][i:i+2], 16) for i in (1, 3, 5))
            secondary_rgb = tuple(int(colors['secondary'][i:i+2], 16) for i in (1, 3, 5))
            
            for x in range(size):
                for y in range(size):
                    if 'magical' in description or 'mystical' in description:
                        distance = ((x - size//2)**2 + (y - size//2)**2)**0.5
                        angle = np.arctan2(y - size//2, x - size//2)
                        spiral = (distance + angle * 10) % 40 / 40
                        color = tuple(int(primary_rgb[i] * spiral + secondary_rgb[i] * (1-spiral)) for i in range(3))
                    elif 'tech' in description or 'mechanical' in description:
                        if (x % 30 < 3) or (y % 30 < 3):
                            color = secondary_rgb
                        else:
                            color = primary_rgb
                    elif 'artistic' in description or 'creative' in description:
                        wave = (np.sin(x * 0.1) + np.cos(y * 0.1)) / 2 + 0.5
                        color = tuple(int(primary_rgb[i] * wave + secondary_rgb[i] * (1-wave)) for i in range(3))
                    else:
                        blend = (x + y) / (size * 2)
                        color = tuple(int(primary_rgb[i] * (1-blend) + secondary_rgb[i] * blend) for i in range(3))
                    
                    pixels[x, y] = color
            
            image.save(texture_path)
            return True
            
        except Exception as e:
            print(f"Failed to generate texture: {e}")
            return False

# Global generator instance
weapon_generator = WeaponGenerator()

@app.post("/generate-arena-weapons", response_model=ArenaResponse)
async def generate_arena_weapons(request: ArenaRequest):
    try:
        arena_id = weapon_generator.generate_arena_id()
        arena_folder = weapon_generator.create_arena_folder(arena_id)
        
        print(f"🎯 Generating MBTI-based weapons for arena: {arena_id}")
        print(f"   Player 1: {request.player1_mbti}")
        print(f"   Player 2: {request.player2_mbti}")
        print(f"   Arena folder: {arena_folder}")
        
        valid_types = list(weapon_generator.mbti_profiles.keys())
        if request.player1_mbti.upper() not in valid_types:
            raise HTTPException(status_code=400, detail=f"Invalid MBTI type: {request.player1_mbti}")
        if request.player2_mbti.upper() not in valid_types:
            raise HTTPException(status_code=400, detail=f"Invalid MBTI type: {request.player2_mbti}")
        
        scenario_data = weapon_generator.generate_scenario_and_weapons(
            request.player1_mbti, request.player2_mbti
        )
        
        weapons = await weapon_generator.generate_3d_weapons(
            scenario_data['weapons'], arena_folder
        )
        
        print(f"✅ Successfully generated {len(weapons)} MBTI-based weapons")
        print(f"📁 All files saved in: {arena_folder}")
        
        return ArenaResponse(
            scenario=scenario_data['scenario'],
            weapons=weapons,
            arena_id=arena_id,
            arena_folder=arena_folder
        )
        
    except Exception as e:
        print(f"❌ Generation failed: {e}")
        raise HTTPException(status_code=500, detail=f"Generation failed: {str(e)}")

@app.get("/health", response_model=ServiceStatus)
async def health_check():
    """Enhanced health check with system stats"""
    stats = weapon_generator.get_system_stats()
    
    return ServiceStatus(
        status="healthy",
        api_url=SERVICE_CONFIG["api_url"],
        public_ip=SERVICE_CONFIG["public_ip"],
        port=SERVICE_CONFIG["port"],
        device=str(weapon_generator.device),
        memory_usage=stats["memory_usage"],
        gpu_memory=stats["gpu_memory"],
        uptime_seconds=stats["uptime_seconds"],
        total_arenas_generated=stats["total_arenas_generated"]
    )

@app.get("/mbti-types")
async def get_mbti_types():
    return {
        mbti: {
            "name": profile["name"],
            "traits": profile["traits"],
            "style": profile["weapon_style"]
        }
        for mbti, profile in weapon_generator.mbti_profiles.items()
    }

@app.post("/ping")
async def ping_aeon_server():
    """Send periodic ping to AEON server"""
    weapon_generator.register_with_aeon_server()
    return {"status": "pinged"}

if __name__ == "__main__":
    import uvicorn
    port = SERVICE_CONFIG["port"]
    print(f"🚀 Starting service on {SERVICE_CONFIG['public_ip']}:{port}")
    print(f"📡 Will register with AEON server: {SERVICE_CONFIG['aeon_server_url']}")
    uvicorn.run(app, host="0.0.0.0", port=port)
EOF
    
    log_success "Enhanced weapon generator service created"
}

register_with_aeon_server() {
    log_info "Registering with AEON server at $AEON_SERVER_URL..."
    
    # Make sure we're in the project directory and the file exists
    if [ ! -f "service_registry.json" ]; then
        log_error "service_registry.json not found in current directory"
        return 1
    fi
    
    # Create registration payload
    REGISTRATION_DATA=$(cat service_registry.json)
    
    # Try to register
    if curl -X POST "$AEON_SERVER_URL/api/weapon-services/register" \
        -H "Content-Type: application/json" \
        -d "$REGISTRATION_DATA" \
        --connect-timeout 10 \
        --silent \
        --fail > /dev/null 2>&1; then
        log_success "Successfully registered with AEON server"
    else
        log_warning "Could not register with AEON server (will retry when service starts)"
    fi
}

# Main setup process
main() {
    log_step "System Requirements Check"
    
    # Check required commands
    for cmd in python3 pip git curl netstat; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is required but not installed"
            exit 1
        fi
    done
    
    # Check optional commands
    if ! command -v dig &> /dev/null; then
        log_warning "dig not found, will use alternative IP detection methods"
    fi
    
    log_step "Network Configuration"
    detect_public_ip
    log_success "AEON server URL: $AEON_SERVER_URL"
    find_available_port
    
    log_step "Creating Project Directory"
    mkdir -p $PROJECT_NAME
    cd $PROJECT_NAME
    log_success "Project directory created: $(pwd)"
    
    # CREATE SERVICE REGISTRY AFTER WE'RE IN THE RIGHT DIRECTORY
    create_service_registry
    
    log_step "Setting up Python Environment"
    
    if [ ! -d "venv" ]; then
        python3 -m venv venv
        log_success "Virtual environment created"
    fi
    
    source venv/bin/activate
    pip install --upgrade pip
    
    log_step "Installing Dependencies"
    install_python_deps
    install_shap_e
    
    log_step "Creating Enhanced Service"
    create_enhanced_weapon_generator
    
    mkdir -p generated_weapons
    
    log_step "Service Registration"
    # NOW register from within the project directory where service_registry.json exists
    register_with_aeon_server
    
    # Create startup scripts
    cat > start_service.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
source venv/bin/activate
python weapon_generator.py
EOF
    chmod +x start_service.sh
    
    # Create a test script
    cat > test_service.sh << EOF
#!/bin/bash
cd "\$(dirname "\$0")"

API_URL="http://$PUBLIC_IP:$SERVICE_PORT"

echo "🔍 Testing AEON Weapon Generator Service..."
echo "Service IP: $PUBLIC_IP"
echo "Service Port: $SERVICE_PORT"
echo "API URL: \$API_URL"
echo "AEON Server: $AEON_SERVER_URL"
echo ""

echo "1. Health Check:"
curl -s "\$API_URL/health" | python3 -m json.tool || echo "❌ Health check failed"
echo ""

echo "2. MBTI Types:"
curl -s "\$API_URL/mbti-types" | python3 -m json.tool || echo "❌ MBTI types failed"
echo ""

echo "3. Generate Arena Test:"
curl -X POST "\$API_URL/generate-arena-weapons" \\
    -H "Content-Type: application/json" \\
    -d '{"player1_mbti": "INTJ", "player2_mbti": "ENFP"}' \\
    -s | python3 -m json.tool || echo "❌ Generation test failed"
EOF
    chmod +x test_service.sh
    
    log_success "Setup complete!"
    
    echo -e "\n${GREEN}╔══════════════════════════════════════════════════════════════╗"
    echo -e "║                  AEON SETUP COMPLETE! 🎉                    ║"
    echo -e "╚══════════════════════════════════════════════════════════════╝${NC}"
    
    API_URL="http://$PUBLIC_IP:$SERVICE_PORT"
    
    echo -e "\n${CYAN}🌐 Service Information:${NC}"
    echo -e "   Public IP: ${YELLOW}$PUBLIC_IP${NC}"
    echo -e "   Port: ${YELLOW}$SERVICE_PORT${NC}"
    echo -e "   API URL: ${YELLOW}$API_URL${NC}"
    echo -e "   Health: ${YELLOW}$API_URL/health${NC}"
    echo -e "   Docs: ${YELLOW}$API_URL/docs${NC}"
    echo -e "   AEON Server: ${YELLOW}$AEON_SERVER_URL${NC}"
    
    echo -e "\n${BLUE}🔧 Available Scripts:${NC}"
    echo -e "   Start service: ${YELLOW}./start_service.sh${NC}"
    echo -e "   Test service: ${YELLOW}./test_service.sh${NC}"
    
    echo -e "\n${BLUE}🚀 Starting service...${NC}"
    echo -e "The service will automatically register with your AEON server!\n"
    
    # Start the service
    python weapon_generator.py
}

# Run main function
main "$@"