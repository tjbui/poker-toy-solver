# Poker Equity Calculator

A full-stack poker equity calculator that allows users to select hole cards, community cards, and simulate outcomes using Monte Carlo.

## Features

- Select **Hero**, **Villain**, and **Community** cards via interactive UI
- Card picker modal with full 52-card deck + face-down option
- Prevents duplicate card selection
- Clean poker-table styled UI
- Designed for fast simulation using a C++ backend (CPU → GPU)

## Tech Stack

**Frontend**
- React + TypeScript + CSS

**Backend**
- C++ HTTP server
- Engine abstraction for CPU/GPU execution

**Simulation Engine**
- C++ Monte Carlo simulation
- CUDA acceleration
