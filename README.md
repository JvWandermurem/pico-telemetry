# Pico W Telemetry 

Firmware embarcado para **Raspberry Pi Pico W** desenvolvido com o **Pico SDK nativo (C/C++)**, capaz de ler sensores, conectar-se ao Wi-Fi e enviar telemetria via HTTP POST para o backend da [Atividade 1](https://github.com/JvWandermurem/sistema-fila).

>  Backend (Atividade 1): [embarcado-backend](https://github.com/JvWandermurem/sistema-fila)

---

## Framework / Toolchain

| Item | Valor |
|---|---|
| Toolchain | **Pico SDK Nativo (C/C++)**  |
| Compilador | `arm-none-eabi-gcc 14.2.0` |
| Build System | `CMake 4.3.1` |
| SDK | `pico-sdk` (master) com suporte a cyw43 + lwIP |
| Sistema Operacional | Arch Linux (Omarchy) |

---

##  Sensores Integrados

### Sensor Analógico - Temperatura Interna (ADC)

| Atributo | Valor |
|---|---|
| Tipo | Sensor de temperatura interno do RP2040 |
| Interface | ADC canal 4 (interno ao chip) |


### Sensor Digital - Botão Simulado (GPIO)

| Atributo | Valor |
|---|---|
| Tipo | Estado digital binário (0 ou 1) |
| Interface | GPIO lógico interno |
| Pino GPIO | Simulado por software (toggle a cada ciclo) |


---
## Estrutura do Projeto

```
pico-telemetry/
├── src/
│   └── main.c              
├── include/
│   └── lwipopts.h          
├── config.h                
├── config.h.example        
├── CMakeLists.txt          
├── pico_sdk_import.cmake  
├── .gitignore
└── README.md
```
---
##  Como rodar?

### Pré-requisitos

```bash
# Arch Linux / Omarchy
sudo pacman -S cmake arm-none-eabi-gcc arm-none-eabi-newlib git python

# Clonar o Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init

# Configurar variável de ambiente (zsh)
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.zshrc
source ~/.zshrc
```

### Configuração

```bash
# Copie o arquivo de configuração de exemplo
cp config.h.example config.h

# Edite com seus dados
nano config.h
```

### Compilação

```bash
# Copie o arquivo de import do SDK
cp $PICO_SDK_PATH/external/pico_sdk_import.cmake .

# Crie a pasta de build e compile
mkdir -p build && cd build
cmake .. -DPICO_BOARD=pico_w -DPICO_SDK_PATH=$PICO_SDK_PATH
make -j4
```

O arquivo `pico-telemetry.uf2` será gerado na pasta `build/`.

### Gravação no Pico W

```bash
# 1. Segure o botão BOOTSEL no Pico W
# 2. Conecte o cabo USB ao PC (mantendo BOOTSEL pressionado)
# 3. Solte o botão — o Pico aparece como dispositivo de armazenamento

# Monte o dispositivo (Linux)
sudo mkdir -p /mnt/pico
sudo mount /dev/sda1 /mnt/pico

# Copie o firmware
sudo cp build/pico-telemetry.uf2 /mnt/pico/

# O Pico reinicia automaticamente e começa a executar
```

---

## Configuração de Rede

Edite o arquivo `config.h` com suas credenciais:

```c
#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID      "nome do seu wifi"
#define WIFI_PASSWORD  "senha"
#define BACKEND_IP     "xxx.xxx.xxx.xxx"   // ex: 192.168.1.0
#define BACKEND_PORT   8080
#define DEVICE_ID      "pico-w-01"

#endif
```

> O Pico W e o computador rodando o backend devem estar na **mesma rede Wi-Fi**.

Para descobrir seu IP local:
```bash
ip addr show | grep "inet " | grep -v 127.0.0.1
```

O backend roda localmente via Docker:
```bash
cd ../embarcado-backend
docker compose up
```
---

## Formato da Telemetria

O firmware envia dados no mesmo formato esperado pelo backend da Atividade 1:

```json
{
  "user_id": "pico-w-01",
  "action": "temperature_28",
  "timestamp": 150
}
```

| Campo | Descrição |
|---|---|
| `user_id` | Identificador do dispositivo Pico W |
| `action` | Tipo de leitura + valor (ex: `temperature_28`, `button_press_1`) |
| `timestamp` | Contador de segundos desde o boot do dispositivo |

---

## Prints de Funcionamento

### Log Serial do Pico W
![print Raspbarry serial](/assets/rasbarry.jpeg)

![print Raspbarry serial + placa](/assets/rasbarry-foto2.jpeg)

### API backend 
![print backend](/assets/backend.jpeg)
---

