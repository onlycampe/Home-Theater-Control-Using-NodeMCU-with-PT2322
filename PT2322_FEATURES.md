# 🎛️ PT2322 - Real Available Features

## 📖 Overview

The **PT2322** is a 6-channel (5.1) audio processor from Princeton Technology with I²C control.

**I²C Address**: 0x44 (write) / 0x45 (read)

⚠️ **IMPORTANT**: The PT2322 is an audio **PROCESSOR**, not a **GENERATOR**. It processes existing audio signals but cannot generate tones or signals on its own.

---

## ✅ REAL PT2322 Features

Based on the official datasheet, these are the **confirmed** chip features:

---

### 1. **Master Volume (6-Channel Master Volume)** ✅
```cpp
pt.setVol(value);  // 0-79
```
- **Range**: 0 to 79
- **Control**: 0 dB to -79 dB (1 dB per step)
- **Project status**: ✅ IMPLEMENTED with web slider
- **Use**: System overall volume

---

### 2. **Individual Channel Attenuation** ✅
```cpp
pt.setCenter_att(value);    // Center
pt.setSub_att(value);       // Subwoofer
pt.setFront_lk_att(value);  // Front Left
pt.setFront_rk_att(value);  // Front Right
pt.setRear_lk_att(value);   // Rear Left
pt.setRear_rk_att(value);   // Rear Right
```
- **Range**: 0 to 15
- **Control**: 0 dB to -15 dB (1 dB per step)
- **Project status**: ✅ IMPLEMENTED (Center, Sub, Front, Rear)
- **Use**: Individual speaker balancing

---

### 3. **Mute** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // mute = 0 or 1
```
- **Values**: 0 (off), 1 (on)
- **Project status**: ✅ IMPLEMENTED with web button
- **Use**: Instantly silence audio
- **Note**: Mute is abrupt (no fade)

---

### 4. **3D Surround Effect** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // ddd = 0 or 1
```
- **Values**: 0 (off), 1 (on)
- **Project status**: ✅ IMPLEMENTED with web button
- **Use**: 3D/spatial audio effect
- **Effect**: Expands virtual sound field
- **Note**: Only on/off, no intensity levels

---

### 5. **3-Band Tone Control (Bass, Middle, Treble)** ✅
```cpp
pt.setBass(value);    // -14 to +14
pt.setMiddle(value);  // -14 to +14
pt.setTreble(value);  // -14 to +14
```
- **Range**: -14 dB to +14 dB
- **Step**: 2 dB per increment
- **Project status**: ❌ Code exists but always at 0
- **Requires**: TEMB=1 to function
- **Frequencies**:
  - Bass: ~100 Hz and below
  - Middle: 1 kHz - 3 kHz
  - Treble: ~10 kHz and above

---

### 6. **Tone Defeat (TEMB - Tone Enhancement Bypass)** ✅
```cpp
pt.setFunc(mute, ddd, temb);  // temb = 0 or 1
```
- **Values**: 
  - 0 = Tone control BYPASS (Bass/Middle/Treble ignored)
  - 1 = Tone control ACTIVE (Bass/Middle/Treble work)
- **Project status**: ❌ Always at 0 (bypass)
- **Use**: Enable/disable equalization circuit
- **Effect**: When 0, Bass/Middle/Treble are ignored

---

## ❌ Features That DON'T EXIST in PT2322

After verifying the official datasheet, the following features **DON'T EXIST** in the chip and have been removed from this documentation:

### Removed (Not real PT2322 features):

1. ❌ **Input Gain Control** - Doesn't exist
2. ❌ **Input Select/Mixing** - Doesn't exist
3. ❌ **Surround Delay** - Doesn't exist
4. ❌ **Speaker Configuration** - Doesn't exist
5. ❌ **Soft Mute** - Doesn't exist (mute is always abrupt)
6. ❌ **Enhanced 3D Settings/Levels** - Doesn't exist (only on/off)
7. ❌ **Dynamic Range Control (DRC)** - Doesn't exist
8. ❌ **Loudness** - Doesn't exist
9. ❌ **Auto Volume Control** - Doesn't exist
10. ❌ **Spectrum Analyzer Output** - Doesn't exist (makes no sense in processor)
11. ❌ **Test Tone Generator** - **DOESN'T EXIST** ⚠️ (PT2322 doesn't generate audio!)
12. ❌ **Phase Inversion** - Doesn't exist

**Conclusion**: The PT2322 is a **SIMPLE and FOCUSED** chip, with only 6 main features.

---

## 📊 Real Feature Summary

### Total Real Features: 6

| # | Feature | Project Status | Complexity |
|---|---------|----------------|------------|
| 1 | Master Volume (0 to -79 dB) | ✅ Implemented | Low |
| 2 | Individual 6-channel Volume (0 to -15 dB) | ✅ Implemented | Low |
| 3 | Mute | ✅ Implemented | Very Low |
| 4 | 3D Surround (on/off) | ✅ Implemented | Low |
| 5 | Bass/Middle/Treble (-14 to +14 dB) | ❌ Disabled | Low |
| 6 | Tone Defeat (TEMB) | ❌ Always in bypass | Very Low |

### Current Implementation: 4/6 (67%) ✅

**Your project uses 67% of the real PT2322 features!**

---

## 🎯 Available But NOT Implemented Features

### Bass/Middle/Treble + TEMB 🎚️

**Only real feature not implemented that's worth it:**

```cpp
// Add variables
int currentBass = 0;    // -14 to +14
int currentMiddle = 0;  // -14 to +14
int currentTreble = 0;  // -14 to +14

// Enable TEMB
pt.setFunc(0, 1, 1);  // mute=0, ddd=1, TEMB=1

// Apply EQ
pt.setBass(currentBass);
pt.setMiddle(currentMiddle);
pt.setTreble(currentTreble);
```

**Advantages:**
- ✅ Adjust sound for your environment
- ✅ Compensate for speaker deficiencies
- ✅ Personal sound preference
- ✅ Very simple to implement

**Cost:** ~12 bytes of RAM

---

## 🔧 How to Add Bass/Middle/Treble

If you want to implement equalization (only real missing feature):

### 1. Add variables (HomeTheater_v2.ino):
```cpp
// After currentRearVol (~line 58)
int currentBass = 0;    // -14 to +14
int currentMiddle = 0;  // -14 to +14
int currentTreble = 0;  // -14 to +14
```

### 2. Modify applySettings():
```cpp
void applySettings() {
    pt.setVol(currentTotalVol);
    pt.setCenter_att(15 - currentCenterVol);
    pt.setSub_att(15 - currentSubVol);
    pt.setFront_lk_att(15 - currentFrontVol);
    pt.setFront_rk_att(15 - currentFrontVol);
    pt.setRear_lk_att(15 - currentRearVol);
    pt.setRear_rk_att(15 - currentRearVol);
    
    // ✅ ENABLE TEMB for EQ to work
    pt.setFunc(0, 1, 1);  // mute=0, ddd=1, TEMB=1
    
    // ✅ Apply EQ
    pt.setBass(currentBass);
    pt.setMiddle(currentMiddle);
    pt.setTreble(currentTreble);
}
```

### 3. Add handler:
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

### 4. Register endpoint:
```cpp
void setupServer() {
    server.on("/", handleRoot);
    server.on("/setVolume", handleSetVolume);
    server.on("/setFunc", HTTP_POST, handleSetFunc);
    server.on("/setEQ", HTTP_POST, handleSetEQ);  // ✅ NEW
    server.on("/status", handleStatus);
    server.on("/status-page", handleStatusPage);
    server.on("/test-tone.html", handleTestPage);
    server.on("/testTone", HTTP_POST, handleTestTone);
    server.on("/stopTest", HTTP_POST, handleStopTest);
    server.begin();
}
```

### 5. Add sliders to interface (webpages.h):
```html
<!-- After existing volume sliders -->
<label class='form-text'>Bass (-14 to +14 dB)</label>
<input type='range' name='bass' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>

<label class='form-text'>Middle (-14 to +14 dB)</label>
<input type='range' name='middle' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>

<label class='form-text'>Treble (-14 to +14 dB)</label>
<input type='range' name='treble' min='-14' max='14' value='0' 
       oninput='updateEQ(this)'>
```

### 6. Add JavaScript function:
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

## ✅ Corrected Conclusion

### The PT2322 is a SIMPLE chip:
- ✅ **6 REAL features** (not 20 as previously documented)
- ✅ **Focus on quality** over quantity of features
- ✅ **Low noise and distortion** guaranteed
- ✅ **High channel separation** (>80 dB)

### Your current project:
- ✅ **Implements 4 of 6** real features (67%)
- ✅ Covers all **essentials**: Volume, Balancing, Mute, 3D
- 🎚️ **Optional**: Bass/Middle/Treble (if you want equalization)

### FALSE features removed from this documentation:
The following features were **removed** because they DON'T exist in PT2322:
- Input Gain, Input Select, Surround Delay, Speaker Config
- Soft Mute, 3D Levels, DRC, Loudness, Auto Volume
- Spectrum Analyzer, **Test Tone Generator**, Phase Inversion

**Final Recommendation**: Your system is **complete and optimized**! If you want to experiment with equalization (Bass/Mid/Treble), it's the only real available but not implemented feature.

---

**Document Updated**: Based on verification of official PT2322 datasheet! 📚✅
