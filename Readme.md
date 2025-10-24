# Módulo B – Sistema IIoT Industrial | CyberNDS

## 🔹 Descrição do Projeto

Este projeto implementa um **sistema IIoT (Industrial Internet of Things)** usando Arduino e Python, com a seguinte funcionalidade:  

- Monitora **temperatura e umidade** em três pontos da linha de produção (Node1, Node2, Node3).  
- Envia dados sem fio via **NRF24L01** para uma estação central.  
- Controla automaticamente atuadores (motor, buzzer) quando condições críticas são detectadas.  
- Permite **controle remoto** dos atuadores via comandos digitados no Arduino receptor.  
- Armazena, analisa e visualiza os dados em tempo real usando Python, detectando anomalias e gerando relatórios.  

---

## 🔹 Componentes Utilizados

### Hardware
- 2× Arduino Uno  
- 2× Módulos NRF24L01 (⚠️ sempre em 3.3 V)  
- 1× Sensor DHT11  
- 1× Micro servo motor  
- 1× Buzzer ativo  
- LEDs verde/vermelho  
- Protoboard + jumpers  
- Capacitor 10 µF (para NRF24L01)  

### Software
- Arduino IDE  
- Python 3.x  
- Bibliotecas Python: `pyserial`, `matplotlib`, `seaborn` (opcional), `json`, `csv`, `fpdf`  

---

## 🔹 Estrutura do Projeto


modulob_cyberNDS/
│
├─ arduino/
│ ├─ transmissor.ino # Código do Arduino transmissor (3 nós lógicos)
│ └─ receptor.ino # Código do Arduino receptor (Estação Central)
│
├─ python/
│ ├─ analise_sensores.py # Script Python de coleta e análise
│ ├─ dados_sensores.csv # Armazena leituras em tempo real
│ └─ anomalias.log # Registro de alertas
│
└─ README.md

---

## 🔹 Funcionalidades

### Arduino Transmissor (`transmissor.ino`)
- Cada Arduino físico simula 3 **nós lógicos** (Node1, Node2, Node3).  
- Node1 possui **atuadores físicos**: servo motor (simula esteira), buzzer e LEDs.  
- Lógica automática:
  - Temperatura > 30°C → para motor e ativa buzzer  
  - Temperatura ≤ 30°C → motor ligado e buzzer desligado  
- Responde a comandos do receptor:
  - `LIGAR_MOTOR` / `DESLIGAR_MOTOR`  
  - `ALARME_ON` / `ALARME_OFF`  

### Arduino Receptor (`receptor.ino`)
- Recebe os dados dos nós via NRF24L01 e imprime no **Monitor Serial** em JSON.  
- Permite envio de comandos para qualquer nó:  


### Python (`analise_sensores.py`)
- Conecta na porta serial do Arduino receptor e lê os dados JSON.  
- Armazena os dados no `dados_sensores.csv`.  
- Detecta anomalias:
- Temperatura > 30°C  
- Umidade < 40% ou > 70%  
- Registra alertas em `anomalias.log`.  
- Atualiza gráficos de **temperatura e umidade em tempo real**.  
- Pode ser expandido para gerar relatórios PDF ou Excel.  

---

## 🔹 Como Usar

1. Abra os códigos Arduino (`transmissor.ino` e `receptor.ino`) na Arduino IDE e **faça upload** para os Arduinos correspondentes.  
2. Conecte o Arduino receptor ao PC via USB.  
3. No VS Code, abra o terminal e execute:
 ```bash
 python python/analise_sensores.py
