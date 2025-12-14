# 🎬 Home Theater Control System v2.5

Sistema de controle de home theater baseado em ESP8266 (NodeMCU) com PT2322, controle via web interface responsiva, comandos de voz (Alexa), controle IR e atualizações OTA.

![Home Theater Control](img/Home.local.png)

---

## ✨ Funcionalidades

### 🌐 Controle Web
- Interface web moderna e responsiva (mobile + desktop)
- Controle de volume total e individual (center, subwoofer, frontal, traseiro)
- Toggle de sistema, Bluetooth, mute e 3D/DDD
- Página de status com informações detalhadas do sistema
- mDNS habilitado (`http://hometheater.local` e `http://home.local`)

### 🎙️ Controle por Voz (Alexa)
- Integração com SinricPro
- Ligar/desligar sistema
- Ajustar volume
- Silenciar/ativar som

### 📡 Outros Recursos
- **OTA Updates**: Atualização de firmware pela rede WiFi (sem cabo USB!)
- **IR Control**: Recepção de comandos infravermelhos
- **WiFi Manager**: Configuração de WiFi via portal AP se não conectar
- **Delays não-bloqueantes**: Sistema sempre responsivo
- **HTML em PROGMEM**: Economiza ~3.5KB de RAM

---

## 🔧 Hardware Necessário

| Componente | Descrição |
|------------|-----------|
| **NodeMCU** | ESP8266 (testado com 4MB Flash) |
| **PT2322** | Controlador de áudio 5.1 via I²C |
| **IR Receiver** | Receptor infravermelho (ex: TSOP4838) |
| **BC547** | Transistor NPN para controle de relés |
| **Fonte ATX** | Alimentação do sistema (standby para NodeMCU) |
| **HD Audio Rush** | Decoder de áudio 5.1 (opcional) |
| **Módulo Bluetooth** | VHM-314 ou similar (opcional) |
| **Relé** | Para controle do Bluetooth |

---

## 📌 Configuração de Pinos

```cpp
D1 (GPIO5)  → I²C SCL (PT2322)
D2 (GPIO4)  → I²C SDA (PT2322)
D3 (GPIO0)  → Relé de controle geral
D4 (GPIO2)  → Receptor IR
D5 (GPIO14) → Audio Input Control (BC547 → HD Audio Rush)
D6 (GPIO12) → Bluetooth Control (Relé)
D7 (GPIO13) → Controle 5.1/2.1
D8 (GPIO15) → PSON (Liga/desliga fonte ATX via BC547)
```

---

## 🚀 Instalação e Configuração

### 1. Requisitos de Software
- [Arduino IDE](https://www.arduino.cc/en/software) 1.8.x ou superior
- **Placas ESP8266**:
  - Abra Arduino IDE → Preferences
  - Em "Additional Board Manager URLs" adicione:
    ```
    http://arduino.esp8266.com/stable/package_esp8266com_index.json
    ```
  - Vá em Tools → Board → Boards Manager
  - Instale "esp8266 by ESP8266 Community" (versão 3.x.x)

### 2. Bibliotecas Necessárias
Instale via Arduino IDE (Tools → Manage Libraries):

```
✅ ESP8266WiFi (incluída no pacote ESP8266)
✅ ESP8266WebServer (incluída no pacote ESP8266)
✅ ESP8266mDNS (incluída no pacote ESP8266)
✅ ArduinoOTA (incluída no pacote ESP8266)
✅ PT2322 → https://github.com/Tiogaplanet/PT2322
✅ IRremoteESP8266 → Instale via Library Manager
✅ SinricPro → Instale via Library Manager
✅ WiFiManager → Instale via Library Manager
```

### 3. Configuração das Credenciais

**IMPORTANTE**: Suas credenciais ficam em um arquivo separado para segurança!

1. **Navegue até** `HomeTheater_v2/`
2. **Copie** `config.example.h` para `config.h`:
   ```bash
   copy config.example.h config.h
   ```
3. **Edite** `config.h` com suas credenciais:

```cpp
// ⚠️ SUAS CREDENCIAIS DO SINRIC PRO
#define SPEAKER_DEVICE_ID       "seu_speaker_id_aqui"
#define SWITCH_DEVICE_ID        "seu_switch_id_aqui"
#define APP_KEY                 "sua_app_key_aqui"
#define APP_SECRET              "seu_app_secret_aqui"

// WiFi AP (quando não conectar)
#define WIFI_AP_PASSWORD        "SuaSenhaForte@2025"

// OTA (atualização remota)
#define OTA_PASSWORD            "HomeTheater@2025"
```

4. **Obtenha credenciais do SinricPro**:
   - Acesse [https://sinric.pro/](https://sinric.pro/)
   - Crie uma conta gratuita
   - Adicione dispositivos: 1x Speaker + 1x Switch
   - Copie os IDs e chaves

### 4. Upload do Firmware

**Primeira vez (via USB):**
1. Conecte o NodeMCU via cabo USB
2. Selecione: Tools → Board → NodeMCU 1.0 (ESP-12E Module)
3. Selecione: Tools → Port → (sua porta COM)
4. Configure: Tools → Flash Size → 4MB (FS:2MB OTA:~1019KB)
5. Clique em Upload (→)

**Atualizações seguintes (via OTA):**
1. Abra Arduino IDE
2. Tools → Port → `HomeTheater at 192.168.x.x`
3. Clique em Upload (→)
4. Digite a senha OTA quando solicitado

---

## 🌐 Acesso ao Sistema

### URLs de Acesso

Após a primeira inicialização, acesse:

```
http://hometheater.local          ← Página principal
http://home.local                 ← Alias alternativo
http://192.168.x.x                ← IP direto (sempre funciona)
```

### Página de Status

```
http://hometheater.local/status-page    ← Interface visual
http://hometheater.local/status         ← API JSON
```

A página de status mostra:
- 🖥️ Informações do sistema (versão, uptime, chip ID)
- ⚡ CPU e Flash (frequência, tamanho, velocidade)
- 💾 Memória RAM (livre, fragmentação)
- 📡 WiFi (SSID, canal, IP, MAC, sinal)
- 🔊 Estado do Home Theater (volumes, mute, etc)

### Se `.local` não funcionar:

**Windows:**
- Instale [Bonjour Print Services](https://support.apple.com/kb/DL999)
- Ou edite `C:\Windows\System32\drivers\etc\hosts`:
  ```
  192.168.x.x    hometheater.local
  192.168.x.x    home.local
  ```

**Linux/Mac:**
- Já suportam mDNS nativamente!

---

## 🎮 Uso do Sistema

### Interface Web

A interface possui 6 botões principais:

| Botão | Função |
|-------|--------|
| 🔌 **Power** | Liga/desliga o sistema completo |
| 📶 **Bluetooth** | Ativa/desativa módulo Bluetooth |
| 🔇 **Mute** | Silencia/ativa áudio |
| 🎲 **3D** | Ativa/desativa efeito 3D/DDD |
| 🎵 **Audio In** | Alterna entrada de áudio |
| 🔊 **5.1/2.1** | Alterna modo de canal |

**Controles de Volume:**
- Volume Total (0-79)
- Center (0-15)
- Subwoofer (0-15)
- Front L/R (0-15)
- Surround L/R (0-15)

### Comandos de Voz (Alexa)

```
"Alexa, liga o home theater"
"Alexa, desliga o home theater"
"Alexa, aumenta o volume do home theater"
"Alexa, diminui o volume do home theater"
"Alexa, coloca o volume do home theater em 50"
"Alexa, silencia o home theater"
```

### Controle IR

Configure seu controle remoto IR e adicione os códigos no código:

```cpp
void processIRCommand(decode_results *results) {
    switch(results->value) {
        case 0xYOUR_CODE_HERE:  // Power
            // Seu código
            break;
        // ... mais comandos
    }
}
```

---

## 🔒 Segurança

### ⚠️ IMPORTANTE: Proteja Suas Credenciais!

O arquivo `config.h` contém informações sensíveis e **NUNCA** deve ser compartilhado ou commitado no Git.

**O que está protegido:**
- ✅ `config.h` está no `.gitignore`
- ✅ Apenas `config.example.h` vai para o GitHub
- ✅ Senhas e chaves ficam no seu dispositivo

**Nunca compartilhe:**
- IDs e chaves do SinricPro
- Senhas de WiFi AP e OTA
- Arquivo `config.h`

**Se acidentalmente expor credenciais:**
1. Altere IMEDIATAMENTE todas as senhas
2. Regenere chaves no SinricPro
3. Remova o arquivo do histórico do Git

---

## 🐛 Solução de Problemas

### NodeMCU não conecta ao WiFi
1. O WiFi Manager cria um AP: "Home Theater"
2. Conecte-se a ele com senha: `@2025` (ou sua senha do config)
3. Configure o WiFi pelo portal web

### OTA não funciona
- Primeira atualização **DEVE ser via USB**
- Verifique se está na mesma rede WiFi
- Confirme a senha OTA no `config.h`

### `.local` não resolve
- Instale Bonjour (Windows)
- Use IP direto: `http://192.168.x.x`
- Edite arquivo hosts

### Sistema reinicia sozinho
- Fonte insuficiente (use fonte de qualidade)
- Verificar conexões (especialmente I²C)
- Monitore RAM na página `/status`

### Logs para Debug

Conecte via Serial Monitor (115200 baud) para ver logs detalhados:
```
[0s] [INFO] 🌟 Iniciando sistema...
[3s] [INFO] ✅ WiFi conectado!
[4s] [INFO] ✅ OTA habilitado!
[6s] [INFO] ✅ [SinricPro] Conectado
```

---

## 📊 Especificações Técnicas

### Performance
- **RAM Livre**: ~30-40KB (de 80KB total)
- **Flash Usada**: ~550KB (de 4MB total)
- **CPU**: 160 MHz
- **Latência Web**: < 100ms (rede local)

### Limitações
- Suporta apenas WiFi 2.4GHz
- Até 4 clientes HTTP simultâneos
- SinricPro: limite de ~60 comandos/minuto (free tier)

---

## 🛠️ Desenvolvimento

### Estrutura do Projeto

```
HomeTheater_v2/
├── HomeTheater_v2.ino     ← Código principal
├── config.h               ← Suas credenciais (NÃO commitar!)
├── config.example.h       ← Template de configuração
├── webpages.h             ← HTML/CSS em PROGMEM
└── .gitignore             ← Proteção de arquivos sensíveis

exampleofthepage.html      ← Preview da interface web
status-page.html           ← Preview da página de status
```

### Compilação

Configurações recomendadas no Arduino IDE:

```
Board:              NodeMCU 1.0 (ESP-12E Module)
Flash Size:         4MB (FS:2MB OTA:~1019KB)
CPU Frequency:      160 MHz
Upload Speed:       115200
Erase Flash:        Only Sketch (primeira vez: All Flash Contents)
```

---

## 🤝 Contribuindo

Contribuições são bem-vindas! Para contribuir:

1. Fork o projeto
2. Crie uma branch: `git checkout -b feature/MinhaFeature`
3. Commit: `git commit -m 'Adiciona MinhaFeature'`
4. Push: `git push origin feature/MinhaFeature`
5. Abra um Pull Request

**Ao contribuir, certifique-se de:**
- Não incluir arquivos `config.h`
- Testar no hardware real
- Documentar mudanças significativas
- Seguir o estilo de código existente

---

## 📝 Changelog

### v2.5 (13/12/2025)
- ✨ Interface web responsiva (mobile + desktop)
- ✨ OTA Updates pela rede
- ✨ Página de status completa com informações do sistema
- ✨ Configurações em arquivo separado (`config.h`)
- ✨ HTML movido para PROGMEM (economia de ~3.5KB RAM)
- ✨ Delays não-bloqueantes
- ✨ mDNS com múltiplos aliases
- 🐛 Correções de estabilidade
- 🔒 Melhorias de segurança

### v1.0 (Original)
- Controle básico via web
- Integração SinricPro
- Controle IR
- PT2322 para áudio 5.1

---

## 📄 Licença

Este projeto está licenciado sob a [MIT License](LICENSE).

Você é livre para usar, modificar e distribuir este projeto, desde que mantenha os créditos originais.

---

## 👤 Autor

**OnlyCampe**
- GitHub: [@onlycampe](https://github.com/onlycampe)

---

## 🙏 Agradecimentos

- Comunidade ESP8266
- Biblioteca PT2322
- IRremoteESP8266
- SinricPro
- WiFiManager

---

## 📞 Suporte

Encontrou um bug ou tem uma sugestão?
- Abra uma [Issue](../../issues)
- Envie um [Pull Request](../../pulls)

---

**⭐ Se este projeto foi útil, deixe uma estrela no GitHub!**

---

**Desenvolvido com ❤️ para entusiastas de Home Theater e IoT**
