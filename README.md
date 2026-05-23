# Novus Hexa-SDR (v1.7)
### An Open-Source, 6-Band QRP Software-Defined Radio Transceiver

---

## 📻 Introduction & Overview

The **Novus Hexa-SDR** is a modular, budget-friendly, 6-band QRP (low-power) high-frequency (HF) amateur radio transceiver designed specifically for student builders, classroom settings, and newly licensed Technician or General Class operators. Built around the powerful dual-core **ESP32-S3 microcontroller** and a vibrant **4.0-inch color LCD display**, this radio seamlessly combines modern software-defined architectures with tactile, traditional analog radio frequency (RF) design.

The radio natively covers the **40, 20, 17, 15, 12, and 10-meter bands** in both **CW (Morse Code)** and **SSB (Voice/USB)** operational modes, delivering an output range dynamically selectable from **0.1 Watts to a full 10 Watts**. 

                              [ ANTENNA BNC ]
                                     │
                                     ▼
+─────────────────────────────────────────────────────────────────────────────────+
│                           INTEGRATED FILTERS SWITCHING                          │
│                      6x Band Pass  /  6x Low Pass Networks                      │
+───────────────────┬─────────────────────────────────────────▲───────────────────+
│ [RX Signal]                             │ [TX RF Carrier]
▼                                         │
+───────────────────────────────────────+         +───────────┴───────────────────+
│        ANALOG DETECTOR PRE-AMP        │         │    POWER AMPLIFIER STAGE      │
│  - FST3253 High-Speed Tayloe Mixer    │         │  - 74ACT08 High-Current Driver│
│  - 74AC74 Dual Phase Clock Splitter   │         │  - IRF510 Switching MOSFET    │
│  - LM358 40dB Differential Amplifier  │         +───────────────────▲───────────+
+───────────────────┬───────────────────+                             │
│                                                 │ [Power Control]
│ (Analog I/Q Baseband Audio)                     │
▼                                                 │
+─────────────────────────────────────────────────────────────────────┴───────────+
│                             CORE PROCESSING UNIT                                │
│           ESP32-S3 Dual-Core Brain  &  Si5351A Clock VFO Generator              │
│  - Core 0: 10kHz Goertzel DSP Decoder    - Core 1: High-Response UI Rendering   │
+─────────────────────────────────────────────────────────────────────────────────+


---

## 🎓 The Educational Mission: Bridging STEM and Ham Radio

The core intent of the Novis Hexa-SDR is to shift ham radio from a hobby of *consumption* to a hobby of *creation*. In an era of tightly packed, machine-soldered, un-serviceable commercial rigs, this project pulls back the curtain on how modern radio frequency hardware actually interacts with digital silicon.

Designed to be accessible to a middle school student or an absolute novice builder, the project splits complex electronics into a **step-by-step modular journey (Kits A through E)**. Through this "built, not bought" philosophy, makers do not just follow instructions—they explore critical engineering principles firsthand:

*   **Software-Defined DSP:** See how the ESP32-S3 runs parallel computing pipelines, tracking a physical tuning knob on one core while executing an audio **Goertzel algorithm** frequency filter on the other.
*   **High-Performance Analog Architecture:** Discover how a **Tayloe Quadrature Sampling Detector** uses high-speed digital multiplexer switches to slice raw electromagnetic waves and down-convert them directly into low-frequency baseband audio.
*   **Electromagnetic Winding:** Learn the tactile art of counting turns and spacing enameled copper wire on carbonyl iron and ferrite toroids to build highly efficient transformers and custom filters.
*   **Circuit Protection & Math:** Wire a hardware directional coupler (Bruene Bridge) to safely rectify high-frequency alternating current into safe DC voltages, letting code compute **Standing Wave Ratio (SWR)** to shield the radio from mismatched antenna loads.

---

## 🌊 The Propagation of Amateur Radio, Morse Code, and Community

Amateur radio is an irreplaceable global utility, serving as a pillar for emergency preparedness, self-reliance, and international community building. When infrastructure fails, the shortwave spectrum remains open, relying only on a piece of wire tossed into a tree and a modest battery pack.

While the Novis Hexa-SDR supports SSB phone operations, it places a heavy emphasis on **CW (Continuous Wave / Morse Code)** by integrating an **on-screen scrolling terminal decoder** reminiscent of elite field rigs like the Elecraft KX3. 

### Why CW Matters
Morse code is the ultimate equalizer in radio communication. Because a CW signal concentrates all its transmitter energy into an incredibly narrow **50 Hz window** (compared to the wide 2.4 kHz required for voice), it cuts through volatile solar conditions and severe atmospheric noise when voice signals fail. Operating at QRP (under 5 Watts) on CW allows an elegant, low-power station to establish solid contacts across continents using a fraction of the energy a household lightbulb consumes. 

By automating the receiver's audio processing through software, the Novis Hexa-SDR provides an interactive learning pad for new operators to hear, copy, and log code in real time out on the field.

---

## 🛠️ Accessible Engineering: Slicing Kit Costs

The largest barrier to entry for youth and newcomers exploring the HF amateur radio spectrum is financial. Elite commercial field radios routinely cost hundreds of dollars—a major investment that pushes out young tinkerers or classrooms operating on tight budgets.

The Novis Hexa-SDR addresses this issue by utilizing highly creative, inexpensive engineering choices:
*   **Class-E Polar Switching:** Rather than using complex, costly linear RF amplifiers to transmit voice, the software splits signals into phase and amplitude. The phase drives a stable, mass-produced **Si5351A clock module**, while the amplitude tracks the supply rail of a rugged, sub-one-dollar industrial switching **IRF510 MOSFET**.
*   **Modular Architecture:** By utilizing discrete breakout modules for complex computing tasks and easy-to-solder through-hole footprints for analog components, students can assemble the rig with a simple, inexpensive soldering iron.
*   **Granular Sourcing:** Sourced sequentially in independent staging blocks, the complete, all-inclusive radio station box—including the internal **3S rechargeable 3200mAh battery pack**, built-in power balance management board, full BNC/GX16 chassis jacks, an extruded aluminum enclosure, and a handheld push-to-talk microphone—can be fully realized for **under $135**. This is a small financial target that can be saved up over just a couple of weekends of basic chores or yard mowing jobs.

---

## 📁 Assembly & Documentation Link

The radio is structured into 5 independent construction kits to ensure smooth benchmarking, isolated troubleshooting, and clear learning milestones:
*   **Kit A:** Core Processing, Display & Operator Interface
*   **Kit B:** SWR & Power Bridge Detection Stage
*   **Kit C:** Integrated Filter Bank Matrix (6x LPF / BPF Channels)
*   **Kit D:** Active RF Mixer, Op-Amp Pre-Amplifier & Transmitter PA
*   **Kit E:** Chassis Layout, Power Storage & Final Enclosure Mechanics

For the step-by-step, comprehensive manufacturing guides, circuit schematics, hardware wiring maps, drilling files, and complete firmware repository, please refer to the master assembly documentation here:

🔗 **[Click here to view the Complete Master Builder's Manual](./build/instructions.md)**

---

*73 and happy building to the next generation of radio designers! de Novus Hexa-SDR Team.*