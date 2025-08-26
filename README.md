# **VSAMPLER: Audio Sampler / Recorder with MIDI & Wi-Fi**

## **Overview**
This project is an **audio sampler/recorder** using the **ESP32-S3** development board (**JC4827W543**) with a **capacitive 4.3" touchscreen**. The system supports **MIDI over USB**, **audio input/output**, **file management on SD card**, **Wi-Fi connectivity**, and **real-time audio processing**.

**Features:**
- **Sampler/Recorder**: Record audio as WAV files, edit, and play back with speed and pitch controls.
- **MIDI**: Support for USB MIDI input (channel selection).
- **Wi-Fi File Server**: Upload/download, rename, or delete audio samples via a simple web server.
- **Audio**: I2S-based audio input/output (using **WM8731 codec**).
- **File Browser**: Manage your samples stored on an SD card.
- **UI**: Simple ASCII-based menu with touch controls.

---

## **Hardware Requirements**
- **ESP32-S3** (JC4827W543) development board.
- **MIKROE-506** audio codec (WM8731) for I2S-based audio input/output.
- **4.3" capacitive touchscreen** for display and input.
- **MicroSD card** for sample storage.
- **Wi-Fi network** for file management via HTTP.
- **Speaker/Headphone output** via I2S to the audio codec.
  
---

## **Software Requirements**
- **Arduino IDE** with **ESP32 core** installed.
- Libraries:
  - **Adafruit_TinyUSB** for MIDI functionality.
  - **ArduinoJson** for JSON file parsing and settings storage.
  - **ESP32-AudioI2S** for handling audio input/output.
  - **LVGL** or **Arduino GFX** for UI rendering.
  - **SD** (ESP32 native) for reading/writing samples on the SD card.
  - **Wire** for I2C communication (used with the WM8731 codec).

---

## **Setup & Installation**

### **1. Install the ESP32 Board in Arduino IDE**
Follow these steps to install the ESP32 core in the Arduino IDE:
- Go to **File > Preferences**.
- Add the following URL to **Additional Boards Manager URLs**:  
  `https://dl.espressif.com/dl/package_esp32_index.json`
- Go to **Tools > Board > Boards Manager**.
- Search for `esp32` and install the latest version.

### **2. Install Required Libraries**
- **Adafruit_TinyUSB**: Install from Arduino Library Manager or from GitHub.
- **ArduinoJson**: Install from Arduino Library Manager.
- **ESP32-AudioI2S**: Install from Arduino Library Manager or from GitHub.
- **LVGL**: Install from Arduino Library Manager.
- **Arduino GFX**: Install from Arduino Library Manager.
- **SD**: Included with the ESP32 core.

---

## **Project Structure**

The project is divided into the following major files:

### **1. `DRUM_2025_VSAMPLER.ino`**
The main entry point for the project, handling:
- Page navigation.
- UI rendering.
- Audio input/output management.

### **2. `audio_hal.ino`**
Handles initialization and configuration of the **WM8731 codec** for I2S audio communication (input/output).

### **3. `audio_output.ino`**
Manages the output route for audio (Jack or Speaker) and volume control.

### **4. `recorder.ino`**
Manages audio recording:
- Records audio as WAV files.
- Handles editing (start/stop/pitch/speed).
- Displays a VU meter and recording duration.

### **5. `wav_loader.ino`**
Loads WAV files from the SD card into RAM and stores them in sample slots.

### **6. `file_browser.ino`**
Allows browsing the SD card, listing, renaming, deleting, and loading WAV samples into the system.

### **7. `wifi_server.ino`**
Manages Wi-Fi connectivity and provides a simple **HTTP file server** for managing audio files on the SD card (upload, download, rename, delete).

### **8. `ui_helpers.ino`**
Contains utility functions for UI rendering, like drawing buttons, text, and other graphical elements.

### **9. `ui_main_menu.ino`**
Defines the main menu UI with buttons for navigating to the sampler, recorder, file browser, settings, MIDI settings, and Wi-Fi page.

### **10. `keyboard.ino`**
Handles the touch keyboard input for renaming files or entering settings like Wi-Fi credentials or sample names.

### **11. `settings_midi.ino`**
Provides a page for configuring the MIDI channel (1-16).

### **12. `settings_persist.ino`**
Handles saving and loading the MIDI channel setting to/from a JSON file stored on the SD card.

### **13. `settings.ino`**
General settings page for managing audio routing (Jack/Speaker), Wi-Fi enable/disable, and saving settings.

---

## **User Interface**

### **Main Menu**
- **SAMPLER**: Access the sample playback and editing functions.
- **RECORDER**: Start recording audio to WAV files.
- **FILES**: Browse and manage the samples stored on the SD card.
- **SETTINGS**: Adjust system settings (audio routing, volume).
- **MIDI**: Select the MIDI channel (1-16).
- **WIFI**: Enable/disable Wi-Fi and manage the HTTP file server.

### **Recorder Page**
- **REC**: Start recording.
- **STOP**: Stop recording.
- **SAVE AS**: Save the current recording with a custom name.
- **EDIT**: Edit the recorded sample (start, stop, pitch, speed).

### **File Browser Page**
- Browse, rename, delete, or load WAV files from the SD card.
- **LOAD**: Load a sample into RAM for playback or editing.
- **UPLOAD**: Upload a file to the SD card via the HTTP server.

---

## **Wi-Fi File Server**
- **SCAN**: Scan for available Wi-Fi networks.
- **CONNECT**: Connect to a Wi-Fi network.
- **SERVER**: Toggle the HTTP file server to allow file uploads/downloads.
- **OFF**: Turn off the Wi-Fi and disconnect from the network.

The **HTTP server** allows you to upload samples from a web browser and store them on the SD card.

---

## **MIDI Integration**
- **MIDI Channel**: Select the MIDI channel for receiving and sending MIDI messages.
- The system supports **MIDI over USB** for controlling the sampler and other parameters.

---

## **Known Limitations**
- **Audio Processing**: All audio processing is handled in **software**. For real-time effects (e.g., delay, reverb), the system may experience some latency or performance issues, depending on the complexity.
- **Wi-Fi and Audio**: Wi-Fi may interfere with real-time audio processing. The system disables Wi-Fi when audio is actively being recorded or played back.
- **File Management**: The SD card can only store a limited number of files (based on available space). Files are stored in the `/samples` directory on the SD card.

---

## **Future Improvements**
- **Effects**: Adding real-time audio effects like reverb, delay, and distortion.
- **Wi-Fi Performance**: Optimizing the Wi-Fi file server for better performance during active audio recording/playback.
- **MIDI Expansion**: Adding MIDI control over other parameters, such as volume, pitch, and effects.

---

## **Acknowledgements**
- **ESP32-S3** development board (JC4827W543).
- **MIKROE-506** (WM8731 audio codec).
- **Arduino** and **ESP32** communities for providing the libraries and tools used in this project.

---