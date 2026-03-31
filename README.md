# Rec Room Cheat Source - FINAL RELEASE

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-83.6%25-blue)](https://isocpp.org/)
[![C](https://img.shields.io/badge/C-16.4%25-blue)](https://www.iso.org/standard/74528.html)

> **Final release following Rec Room's shutdown announcement** - Complete source code with updated offsets

## 📜 Overview

This repository contains the complete source code for the Rec Room cheat tool, released as the game announces its shutdown. The cheat includes fully updated offsets to ensure functionality in the final versions of Rec Room.

## 🛠️ Tools & Methodology

### Offset Discovery Process
To maintain this cheat across Rec Room updates, I developed two companion tools:

#### 1. [RecRoom-Dumper](https://github.com/Longno242/RecRoom-Dumper)
- **Purpose**: Dynamic offset dumper for Rec Room
- **Function**: Automatically extracts and updates offsets for each game version
- **Usage**: Compile into DLL, inject into Rec Room, dump generates in game directory
- **Tech**: Built with C++ (99.8%) and C (0.2%)

#### 2. [Recroom-Referee-Offsets-Automator](https://github.com/Longno242/Recroom-Referee-Offsets-Automator)
- **Purpose**: Specialized tool for referee detection offsets
- **Function**: Automates the process of finding and updating anti-cheat/referee detection offsets
- **Tech**: Pure C++ implementation

### Workflow
