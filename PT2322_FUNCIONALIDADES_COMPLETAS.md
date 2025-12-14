# 🎛️ PT2322 - Funcionalidades Reais Disponíveis

## 📖 Visão Geral

O **PT2322** é um processador de áudio 6 canais (5.1) da Princeton Technology com controle via I²C.

**Endereço I²C**: 0x44 (write) / 0x45 (read)

⚠️ **IMPORTANTE**: O PT2322 é um **PROCESSADOR** de áudio, não um **GERADOR**. Ele processa sinais de áudio existentes, mas não gera tons ou sinais por conta própria.

---

## ✅ Funcionalidades REAIS do PT2322

Baseado no datasheet oficial, estas são as funcionalidades **confirmadas** do chip:

---

### 1. **Volume Master (6-Channel Master Volume)** ✅
```cpp
pt.setVol(value);  // 0-79
```
- **Range**: 0 a 79
- **Controle**: 0 dB a -79 dB (1 dB por step)
- **Status no projeto**: ✅ IMPLEMENTADO com slider web
- **Uso**: Volume geral do sistema

---

### 2. **Atenuação Individual de Canais (Individual Channel Volume)** ✅
```cpp
pt.setCenter_att(value);    // Center
pt.setSub_att(value);       // Subwoofer
pt.setFront_lk_att(value);  // Front Left
pt.setFront_rk_att(value);  // Front Right
pt.setRear_lk_att(value);   // Rear Left
pt.setRear_rk_att(value);   // Rear Right
```
- **Range**: 0 a 15
- **Controle**: 0 dB a -15 dB (1 dB por step)
- **Status no projeto**: ✅ IMPLEMENTADO (Center, Sub, Front, Rear)
- **Uso**: Balanceamento individual de caixas

---

### 3. **Mute** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // mute = 0 ou 1
```
- **Valores**: 0 (off), 1 (on)
- **Status no projeto**: ✅ IMPLEMENTADO com botão web
- **Uso**: Silenciar áudio instantaneamente
- **Nota**: Mute é abrupto (sem fade)

---

### 4. **3D Surround Effect (3D Sound Enhancement)** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // ddd = 0 ou 1
```
- **Valores**: 0 (off), 1 (on)
- **Status no projeto**: ✅ IMPLEMENTADO com botão web
- **Uso**: Efeito 3D/espacial no áudio
- **Efeito**: Expande campo sonoro virtual
- **Nota**: Apenas on/off, sem níveis de intensidade

---

### 5. **3-Band Tone Control (Bass, Middle, Treble)** ✅
```cpp
pt.setBass(value);    // -14 a +14
pt.setMiddle(value);  // -14 a +14
pt.setTreble(value);  // -14 a +14
```
- **Range**: -14 dB a +14 dB
- **Step**: 2 dB por incremento
- **Status no projeto**: ❌ Código existe mas sempre em 0
- **Requer**: TEMB=1 para funcionar
- **Frequências**:
  - Bass: ~100 Hz e abaixo
  - Middle: 1 kHz - 3 kHz
  - Treble: ~10 kHz e acima

---

### 6. **Tone Defeat (TEMB - Tone Enhancement Bypass)** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // temb = 0 ou 1
```
- **Valores**: 
  - 0 = Tone control BYPASS (Bass/Middle/Treble ignorados)
  - 1 = Tone control ATIVO (Bass/Middle/Treble funcionam)
- **Status no projeto**: ❌ Sempre em 0 (bypass)
- **Uso**: Ativa/desativa circuito de equalização
- **Efeito**: Quando 0, Bass/Middle/Treble são ignorados

---

## ❌ Funcionalidades NÃO EXISTEM no PT2322

Após verificação do datasheet oficial, as seguintes funcionalidades **NÃO EXISTEM** no chip e foram removidas desta documentação:

### Removidas (Não são features reais do PT2322):

1. ❌ **Input Gain Control** - Não existe
2. ❌ **Input Select/Mixing** - Não existe
3. ❌ **Surround Delay** - Não existe
4. ❌ **Speaker Configuration** - Não existe
5. ❌ **Soft Mute** - Não existe (mute é sempre abrupto)
6. ❌ **Enhanced 3D Settings/Levels** - Não existe (apenas on/off)
7. ❌ **Dynamic Range Control (DRC)** - Não existe
8. ❌ **Loudness** - Não existe
9. ❌ **Auto Volume Control** - Não existe
10. ❌ **Spectrum Analyzer Output** - Não existe (não faz sentido em processador)
11. ❌ **Test Tone Generator** - **NÃO EXISTE** ⚠️ (PT2322 não gera áudio!)
12. ❌ **Phase Inversion** - Não existe

**Conclusão**: O PT2322 é um chip **SIMPLES e FOCADO**, com apenas 6 funcionalidades principais.

---

## 📊 Resumo Real de Funcionalidades

### Total de Funcionalidades Reais: 6

| # | Funcionalidade | Status no Projeto | Complexidade |
|---|----------------|-------------------|--------------|
| 1 | Volume Master (0 a -79 dB) | ✅ Implementado | Baixa |
| 2 | Volume Individual 6 canais (0 a -15 dB) | ✅ Implementado | Baixa |
| 3 | Mute | ✅ Implementado | Muito Baixa |
| 4 | 3D Surround (on/off) | ✅ Implementado | Baixa |
| 5 | Bass/Middle/Treble (-14 a +14 dB) | ❌ Desabilitado | Baixa |
| 6 | Tone Defeat (TEMB) | ❌ Sempre em bypass | Muito Baixa |

### Implementação Atual: 4/6 (67%) ✅

**Seu projeto usa 67% das funcionalidades reais do PT2322!**

---

## 🎯 Funcionalidades Disponíveis Mas NÃO Implementadas

### Bass/Middle/Treble + TEMB 🎚️

**Única funcionalidade real não implementada que vale a pena:**

```cpp
// Adicionar variáveis
int currentBass = 0;    // -14 a +14
int currentMiddle = 0;  // -14 a +14
int currentTreble = 0;  // -14 a +14

// Ativar TEMB
pt.setFunc(0, 1, 1);  // mute=0, ddd=1, TEMB=1

// Aplicar EQ
pt.setBass(currentBass);
pt.setMiddle(currentMiddle);
pt.setTreble(currentTreble);
```

**Vantagens:**
- ✅ Ajusta som para seu ambiente
- ✅ Compensa deficiências de caixas
- ✅ Preferência pessoal de som
- ✅ Muito simples de implementar

**Custo:** ~12 bytes de RAM

---

## 🔧 Como Adicionar Bass/Middle/Treble

Se quiser implementar a equalização (única feature real faltando):

### 1. Adicionar variáveis (HomeTheater_v2.ino):
```cpp
// Após currentRearVol (~linha 58)
int currentBass = 0;    // -14 a +14
int currentMiddle = 0;  // -14 a +14
int currentTreble = 0;  // -14 a +14
```

### 2. Modificar applySettings():
```cpp
void applySettings() {
    pt.setVol(currentTotalVol);
    pt.setCenter_att(15 - currentCenterVol);
    pt.setSub_att(15 - currentSubVol);
    pt.setFront_lk_att(15 - currentFrontVol);
    pt.setFront_rk_att(15 - currentFrontVol);
    pt.setRear_lk_att(15 - currentRearVol);
    pt.setRear_rk_att(15 - currentRearVol);
    
    // ✅ ATIVAR TEMB para EQ funcionar
    pt.setFunc(0, 1, 1);  // mute=0, ddd=1, TEMB=1
    
    // ✅ Aplicar EQ
    pt.setBass(currentBass);
    pt.setMiddle(currentMiddle);
    pt.setTreble(currentTreble);
}
```

### 3. Adicionar handler:
```cpp
void handleSetEQ() {
    if (server.hasArg("bass")) {
        currentBass = constrain(server.arg("bass").toInt(), -14, 14);
        pt.setBass(currentBass);
    }
    if (server.hasArg("middle")) {
        currentMiddle = constrain(server.arg("middle").toInt(), -14, 14);
        pt.setMiddle(currentMiddle);
    }
    if (server.hasArg("treble")) {
        currentTreble = constrain(server.arg("treble").toInt(), -14, 14);
        pt.setTreble(currentTreble);
    }
    server.send(200, "text/plain", "EQ updated");
}
```

### 4. Registrar endpoint:
```cpp
void setupServer() {
    server.on("/", handleRoot);
    server.on("/setVolume", handleSetVolume);
    server.on("/setFunc", HTTP_POST, handleSetFunc);
    server.on("/setEQ", HTTP_POST, handleSetEQ);  // ✅ NOVO
    server.on("/status", handleStatus);
    server.on("/status-page", handleStatusPage);
    server.on("/test-tone.html", handleTestPage);
    server.on("/testTone", HTTP_POST, handleTestTone);
    server.on("/stopTest", HTTP_POST, handleStopTest);
    server.begin();
}
```

### 5. Adicionar sliders na interface (webpages.h):
```html
<!-- Após os sliders de volume existentes -->
<label class='form-text'>Bass (-14 a +14 dB)</label>
<input type='range' name='bass' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>

<label class='form-text'>Middle (-14 a +14 dB)</label>
<input type='range' name='middle' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>

<label class='form-text'>Treble (-14 a +14 dB)</label>
<input type='range' name='treble' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>
```

### 6. Adicionar função JavaScript:
```javascript
function updateEQ(el) {
    fetch('/setEQ', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `${el.name}=${el.value}`
    });
}
```

---

## ✅ Conclusão Corrigida

### O PT2322 é um chip SIMPLES:
- ✅ **6 funcionalidades REAIS** (não 20 como documentado anteriormente)
- ✅ **Foco em qualidade** sobre quantidade de features
- ✅ **Baixo ruído e distorção** garantidos
- ✅ **Alta separação de canais** (>80 dB)

### Seu projeto atual:
- ✅ **Implementa 4 de 6** funcionalidades reais (67%)
- ✅ Cobre todas as **essenciais**: Volume, Balanceamento, Mute, 3D
- 🎚️ **Opcional**: Bass/Middle/Treble (se quiser equalização)

### Funcionalidades FALSAS removidas desta documentação:
As seguintes funcionalidades foram **removidas** por NÃO existirem no PT2322:
- Input Gain, Input Select, Surround Delay, Speaker Config
- Soft Mute, 3D Levels, DRC, Loudness, Auto Volume
- Spectrum Analyzer, **Test Tone Generator**, Phase Inversion

**Recomendação Final**: Seu sistema está **completo e otimizado**! Se quiser experimentar equalização (Bass/Mid/Treble), é a única feature real disponível mas não implementada.

---

**Documento Atualizado**: Baseado em verificação do datasheet oficial do PT2322! 📚✅
