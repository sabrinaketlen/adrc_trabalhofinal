import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats
import re

PASTA_RESULTADOS = '../scratch/resultados'
FATORES = ['mobilidade', 'quantidade_nos', 'velocidade']
ALGORITMOS = ['AODV', 'OLSR', 'DSDV']
METRICAS = ['perda_pacotes', 'delay_medio', 'control_overhead']
NIVEIS_CONFIANCA = 0.90  

dados = []
for arquivo in os.listdir(PASTA_RESULTADOS):
    if arquivo.endswith('.txt'):
        partes = arquivo.replace('.txt', '').split('_')
        qtd_nos = partes[0].replace('nodes', '')
        mobilidade = partes[1]
        velocidade = partes[2].replace('mps', '')
        algoritmo = partes[3]
        
        with open(os.path.join(PASTA_RESULTADOS, arquivo), 'r') as f:
            conteudo_arquivo = f.read()
        padrao = re.compile(r'PacketLossRatio:\s([\d.]+)%.*AvgDelay:\s([\d.]+)\s.*ControlOverhead:\s([\d.]+)%')
    
       
        correspondencias = padrao.findall(conteudo_arquivo)
        
        
        metricas = []
        for correspondencia in correspondencias:
            perda_pacotes = float(correspondencia[0]) 
            delay_medio = float(correspondencia[1]) 
            overhead = float(correspondencia[2])  
            
            metricas = {
                'perda_pacotes': perda_pacotes,
                'delay_medio': delay_medio,
                'control_overhead': overhead
            }

            dados.append({
                'algoritmo': algoritmo,
                'mobilidade': mobilidade,
                'quantidade_nos': qtd_nos,
                'velocidade': velocidade,
                **metricas
            })

df = pd.DataFrame(dados)


def calcular_ic(dados, confianca=NIVEIS_CONFIANCA):
    media = np.mean(dados)
    desvio_padrao = np.std(dados, ddof=1)
    n = len(dados)
    intervalo = stats.t.interval(confianca, n-1, loc=media, scale=desvio_padrao/np.sqrt(n))
    return media, intervalo[0], intervalo[1]

for algoritmo in ALGORITMOS:
    for fator in FATORES:
        # Filtrar dados do algoritmo
        df_algoritmo = df[df['algoritmo'] == algoritmo]
        
        # Agrupar por nível do fator
        grupos = df_algoritmo.groupby(fator)
        
        
        niveis = sorted(grupos.groups.keys())
        metricas_media = {metrica: [] for metrica in METRICAS}
        metricas_ic_inferior = {metrica: [] for metrica in METRICAS}
        metricas_ic_superior = {metrica: [] for metrica in METRICAS}
        
        for nivel in niveis:
            grupo = grupos.get_group(nivel)
            for metrica in METRICAS:
                media, ic_inf, ic_sup = calcular_ic(grupo[metrica])
                metricas_media[metrica].append(media)
                metricas_ic_inferior[metrica].append(ic_inf)
                metricas_ic_superior[metrica].append(ic_sup)
        
        
        for metrica in METRICAS:
            plt.figure(figsize=(10, 5))
            plt.errorbar(
                niveis, 
                metricas_media[metrica], 
                yerr=[np.array(metricas_media[metrica]) - np.array(metricas_ic_inferior[metrica]), 
                      np.array(metricas_ic_superior[metrica]) - np.array(metricas_media[metrica])],
                fmt='-o', 
                capsize=5,
                label=f'{algoritmo} (IC 90%)'
            )
            plt.title(f'{metrica.replace("_", " ").title()} por {fator.replace("_", " ").title()} - {algoritmo}')
            plt.xlabel(fator.replace("_", " ").title())
            plt.ylabel(metrica.replace("_", " ").title())
            plt.grid(True)
            plt.legend()

            plt.savefig(f'../graficos/{algoritmo}_{fator}_{metrica}.png')
            plt.close()


for metrica in METRICAS:
    plt.figure(figsize=(10, 6))  
    barras = []
    posicoes = np.arange(len(ALGORITMOS)) 
    
   
    cores = ['#FF6347', '#4682B4', '#32CD32'] 
    
    for i, algoritmo in enumerate(ALGORITMOS):
        dados_algoritmo = df[df['algoritmo'] == algoritmo][metrica]
        media, ic_inf, ic_sup = calcular_ic(dados_algoritmo)
        barra = plt.bar(
            x=posicoes[i],  
            height=media, 
            yerr=[[media - ic_inf], [ic_sup - media]],
            capsize=10, 
            label=algoritmo,
            color=cores[i],  
            alpha=0.8, 
            edgecolor='black', 
            linewidth=1.2  
        )
        barras.append(barra)
    
    plt.title(f'{metrica.replace("_", " ").title()} - Comparação Global', fontsize=16, pad=20)
    plt.ylabel(metrica.replace("_", " ").title(), fontsize=14)
    plt.xticks(posicoes, ALGORITMOS, fontsize=12, rotation=0)  # Rótulos no eixo X
    plt.yticks(fontsize=12)
    plt.grid(True, axis='y', linestyle='--', alpha=0.7)  # Grid horizontal suave
    
    for barra in barras:
        for rect in barra:
            height = rect.get_height()
            plt.text(
                rect.get_x() + rect.get_width() / 2.0, 
                height, 
                f'{height:.2f}', 
                ha='center', 
                va='bottom', 
                fontsize=12
            )
    
    plt.legend(fontsize=12, loc='upper right', bbox_to_anchor=(1, 1))
    
    plt.tight_layout()

    plt.savefig(f'../graficos/comparacao_global_{metrica}.png', dpi=300, bbox_inches='tight')
    plt.close()