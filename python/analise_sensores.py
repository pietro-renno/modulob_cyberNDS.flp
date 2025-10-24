# --------------------------------------------------------------------------- #
#            🧠 PROJETO TÉCNICO – SENAI DEV EXPERIENCE | MÓDULO B (IIoT)        #
#                 MODULOB_CYBERNDS - Script de Análise IIoT                     #
# --------------------------------------------------------------------------- #
#   Funções:                                                                  #
#   1. Lê dados JSON da porta serial do Arduino receptor.                     #
#   2. Armazena os dados em 'dados_sensores.csv'.                             #
#   3. Detecta e registra anomalias em 'anomalias.log'.                       #
#   4. Plota gráficos de temperatura e umidade em tempo real.                 #
#   5. Gera um relatório final em PDF com estatísticas e gráficos.            #
# --------------------------------------------------------------------------- #

import serial
import time
import csv
import os
import logging
import json # Biblioteca para processar o formato JSON
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from fpdf import FPDF

# --- CONFIGURAÇÕES ---
# ⚠️ Altere para a porta serial correta do seu Arduino Receptor
PORTA_SERIAL = 'COM8'  
# ⚠️ Ajustado para a mesma velocidade do seu código receptor.ino
BAUD_RATE = 115200 
TIMEOUT_SERIAL = 2

# Arquivos de saída (serão criados na mesma pasta do script)
ARQUIVO_CSV = 'dados_sensores.csv'
ARQUIVO_LOG = 'anomalias.log'
ARQUIVO_RELATORIO = 'relatorio.pdf'
GRAFICO_TEMPERATURA = 'grafico_temperatura.png'
GRAFICO_UMIDADE = 'grafico_umidade.png'

# Limites para detecção de anomalias
TEMP_MAX_ANOMALIA = 30.0
UMID_MIN_ANOMALIA = 40.0
UMID_MAX_ANOMALIA = 70.0

# --- INICIALIZAÇÃO ---

def inicializar_arquivos():
    """Cria o CSV com cabeçalho e configura o logger se não existirem."""
    if not os.path.exists(ARQUIVO_CSV):
        with open(ARQUIVO_CSV, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'id_do_no', 'temperatura', 'umidade'])
    
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(message)s',
        handlers=[
            logging.FileHandler(ARQUIVO_LOG),
            logging.StreamHandler()
        ]
    )

def conectar_serial():
    """Tenta conectar à porta serial e lida com reconexões automáticas."""
    while True:
        try:
            ser = serial.Serial(PORTA_SERIAL, BAUD_RATE, timeout=TIMEOUT_SERIAL)
            logging.info(f"Conectado com sucesso à porta {PORTA_SERIAL} em {BAUD_RATE} baud.")
            return ser
        except serial.SerialException:
            logging.warning(f"Porta {PORTA_SERIAL} não encontrada. Tentando novamente em 5 segundos...")
            time.sleep(5)

# --- LÓGICA PRINCIPAL ---

def ler_e_processar_dados(ser):
    """Lê uma linha JSON da serial, processa, armazena e detecta anomalias."""
    try:
        if not ser.in_waiting: # Verifica se há dados para ler
            return

        linha_bytes = ser.readline()
        linha = linha_bytes.decode('utf-8').strip()

        if linha:
            # Converte a string JSON em um dicionário Python
            dados = json.loads(linha)

            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            node_id = dados['nodeID']
            temperatura = float(dados['temperature'])
            umidade = float(dados['humidity'])
            
            # Armazena os dados no CSV
            with open(ARQUIVO_CSV, 'a', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                writer.writerow([timestamp, node_id, temperatura, umidade])

            print(f"Dados recebidos: {timestamp}, {node_id}, Temp: {temperatura}°C, Umid: {umidade}%")

            verificar_anomalias(timestamp, node_id, temperatura, umidade)

    except json.JSONDecodeError:
        logging.error(f"Erro ao decodificar JSON. Linha recebida: '{linha}'")
    except (UnicodeDecodeError, KeyError, ValueError) as e:
        logging.error(f"Erro ao processar dados: {e}. Linha: '{linha_bytes}'")
    except serial.SerialException:
        logging.error("Erro na comunicação serial. Tentando reconectar...")
        ser.close()
        ser = conectar_serial()

def verificar_anomalias(timestamp, node_id, temp, umid):
    """Verifica se os dados estão fora dos limites e registra no log."""
    if temp > TEMP_MAX_ANOMALIA:
        logging.warning(f"[ANOMALIA] Nó: {node_id} - Superaquecimento! Temperatura: {temp}°C")
    
    if umid < UMID_MIN_ANOMALIA:
        logging.warning(f"[ANOMALIA] Nó: {node_id} - Umidade Baixa! Umidade: {umid}%")
    elif umid > UMID_MAX_ANOMALIA:
        logging.warning(f"[ANOMALIA] Nó: {node_id} - Umidade Alta! Umidade: {umid}%")


# --- VISUALIZAÇÃO EM TEMPO REAL ---

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

def animar_grafico(i):
    """Função que atualiza os gráficos em tempo real lendo o CSV."""
    try:
        data = pd.read_csv(ARQUIVO_CSV)
        data['timestamp'] = pd.to_datetime(data['timestamp'])
        data.set_index('timestamp', inplace=True)

        ax1.cla()
        ax2.cla()

        for node in data['id_do_no'].unique():
            node_data = data[data['id_do_no'] == node]
            ax1.plot(node_data.index, node_data['temperatura'], label=f'{node} Temp', marker='.')
            ax2.plot(node_data.index, node_data['umidade'], label=f'{node} Umid', marker='.')

        # Formatação do Gráfico 1 (Temperatura)
        ax1.set_title('Monitoramento de Temperatura em Tempo Real')
        ax1.set_ylabel('Temperatura (°C)')
        ax1.axhline(y=TEMP_MAX_ANOMALIA, color='r', linestyle='--', label=f'Limite ({TEMP_MAX_ANOMALIA}°C)')
        ax1.legend()
        ax1.grid(True)

        # Formatação do Gráfico 2 (Umidade)
        ax2.set_title('Monitoramento de Umidade em Tempo Real')
        ax2.set_xlabel('Tempo')
        ax2.set_ylabel('Umidade (%)')
        ax2.axhline(y=UMID_MIN_ANOMALIA, color='orange', linestyle='--', label=f'Limites ({UMID_MIN_ANOMALIA}%-{UMID_MAX_ANOMALIA}%)')
        ax2.axhline(y=UMID_MAX_ANOMALIA, color='orange', linestyle='--')
        ax2.legend()
        ax2.grid(True)
        
        plt.tight_layout()

    except FileNotFoundError:
        pass # Ignora se o arquivo ainda não foi criado
    except pd.errors.EmptyDataError:
        pass # Ignora se o arquivo CSV estiver vazio

# --- GERAÇÃO DE RELATÓRIO FINAL ---

def gerar_relatorio_final():
    """Calcula estatísticas, gera gráficos e cria um relatório completo em PDF."""
    logging.info("Iniciando a geração do relatório final...")
    
    try:
        df = pd.read_csv(ARQUIVO_CSV)
        if df.empty:
            logging.warning("O arquivo de dados está vazio. Relatório não pode ser gerado.")
            return

        # 1. Estatísticas
        estatisticas = df.groupby('id_do_no').agg(
            temp_media=('temperatura', 'mean'), temp_max=('temperatura', 'max'), temp_min=('temperatura', 'min'),
            umid_media=('umidade', 'mean'), umid_max=('umidade', 'max'), umid_min=('umidade', 'min')
        ).round(2)

        # 2. Contagem de anomalias
        with open(ARQUIVO_LOG, 'r', encoding='utf-8') as f:
            num_anomalias = sum(1 for line in f if '[ANOMALIA]' in line)

        # 3. Gráficos de tendência (salvos como imagem para o PDF)
        fig, ax = plt.subplots(figsize=(10, 5))
        for node in df['id_do_no'].unique():
            df[df['id_do_no'] == node].plot(x='timestamp', y='temperatura', ax=ax, label=node, rot=15)
        ax.set_title('Tendência Histórica de Temperatura por Nó'); ax.set_xlabel('Tempo'); ax.set_ylabel('Temperatura (°C)'); ax.grid(True)
        plt.tight_layout(); fig.savefig(GRAFICO_TEMPERATURA)
        plt.close(fig)
        
        fig, ax = plt.subplots(figsize=(10, 5))
        for node in df['id_do_no'].unique():
            df[df['id_do_no'] == node].plot(x='timestamp', y='umidade', ax=ax, label=node, rot=15)
        ax.set_title('Tendência Histórica de Umidade por Nó'); ax.set_xlabel('Tempo'); ax.set_ylabel('Umidade (%)'); ax.grid(True)
        plt.tight_layout(); fig.savefig(GRAFICO_UMIDADE)
        plt.close(fig)

        # 4. Criação do PDF
        pdf = FPDF()
        pdf.add_page()
        pdf.set_font('Arial', 'B', 16); pdf.cell(0, 10, 'Relatório de Monitoramento IIoT - CyberNDS', 0, 1, 'C'); pdf.ln(10)
        
        pdf.set_font('Arial', 'B', 12); pdf.cell(0, 10, '1. Estatísticas Gerais por Nó', 0, 1)
        pdf.set_font('Courier', '', 10); pdf.multi_cell(0, 5, estatisticas.to_string()); pdf.ln(5)

        pdf.set_font('Arial', 'B', 12); pdf.cell(0, 10, '2. Resumo de Anomalias', 0, 1)
        pdf.set_font('Arial', '', 11); pdf.cell(0, 5, f'Total de anomalias registradas no log: {num_anomalias}', 0, 1); pdf.ln(5)

        pdf.set_font('Arial', 'B', 12); pdf.cell(0, 10, '3. Sugestões Automáticas', 0, 1)
        pdf.set_font('Arial', '', 11)
        for node_id, row in estatisticas.iterrows():
            if row['temp_max'] > TEMP_MAX_ANOMALIA:
                pdf.multi_cell(0, 5, f"- {node_id} apresentou picos de superaquecimento ({row['temp_max']} C). Sugestão: Verificar sistema de ventilação ou carga de trabalho do equipamento associado.")
        if num_anomalias == 0:
            pdf.cell(0, 5, '- Sistema operou dentro dos parâmetros normais durante o período de monitoramento.', 0, 1)
        pdf.ln(10)

        pdf.add_page(); pdf.set_font('Arial', 'B', 14); pdf.cell(0, 10, '4. Gráficos de Tendência', 0, 1, 'C'); pdf.ln(5)
        pdf.image(GRAFICO_TEMPERATURA, x=10, w=190)
        pdf.image(GRAFICO_UMIDADE, x=10, w=190)

        pdf.output(ARQUIVO_RELATORIO)
        logging.info(f"Relatório '{ARQUIVO_RELATORIO}' gerado com sucesso!")

    except Exception as e:
        logging.error(f"Ocorreu um erro ao gerar o relatório: {e}")

# --- EXECUÇÃO ---
if __name__ == "__main__":
    inicializar_arquivos()
    ser = conectar_serial()

    ani = FuncAnimation(fig, animar_grafico, interval=2000)
    plt.show(block=False) 

    try:
        while True:
            ler_e_processar_dados(ser)
            plt.pause(0.1) # Pausa curta para permitir que a interface gráfica seja atualizada
    except KeyboardInterrupt:
        print("\nPrograma interrompido pelo usuário.")
    finally:
        if ser.is_open:
            ser.close()
            print("Conexão serial fechada.")
        
        gerar = input("Deseja gerar o relatório final em PDF? (s/n): ")
        if gerar.lower() == 's':
            gerar_relatorio_final()