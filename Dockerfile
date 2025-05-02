# =============================================================================
# Dockerfile - Configuração de build para a Rinha de Backend 2023
# 
# Versão: 1.0.0
# Data: 28/04/2025
# 
# Responsabilidades:
# - Definir ambiente de execução
# - Instalar dependências
# - Compilar componentes
# - Configurar entrypoint
# 
# Equipe:
# - Zeh Sobrinho (Product Owner)
# - Manus (Full Stack AGI DevOp)
# =============================================================================

FROM ubuntu:22.04

LABEL maintainer="Manus <manus@example.com>"
LABEL version="1.0.0"
LABEL description="Rinha de Backend 2023 - Bash como Orquestrador"

# Evitar interações durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# Instalar dependências
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    bash \
    netcat \
    curl \
    uuid-dev \
    apache2-utils \
    jq \
    && rm -rf /var/lib/apt/lists/*

# Criar diretórios de trabalho
WORKDIR /app
RUN mkdir -p /app/data /app/logs

# Copiar arquivos de código
COPY src/ /app/src/
COPY worker/ /app/worker/
COPY database/ /app/database/
COPY scripts/ /app/scripts/
COPY nginx/ /app/nginx/

# Tornar os scripts executáveis
RUN chmod +x /app/src/*.sh
RUN chmod +x /app/scripts/*.sh

# Compilar os componentes C
RUN cd /app/worker && gcc -O3 -march=native -flto -pthread -o worker worker.c -luuid
RUN cd /app/database && gcc -O3 -march=native -flto -pthread -o database database.c

# Expor porta da API
EXPOSE 9999

# Comando para iniciar o orquestrador
CMD ["/app/src/orchestrator.sh"]

