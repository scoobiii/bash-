#!/bin/bash

# =============================================================================
# build.sh - Script de compilação para a Rinha de Backend 2023
# 
# Versão: 1.0.0
# Data: 28/04/2025
# 
# Responsabilidades:
# - Compilar os componentes C do projeto
# - Verificar dependências
# - Otimizar binários para performance
# 
# Equipe:
# - Zeh Sobrinho (Product Owner)
# - Manus (Full Stack AGI DevOp)
# =============================================================================

# Verificar dependências
if ! command -v gcc &> /dev/null; then
    echo "Erro: GCC não encontrado"
    exit 1
fi

# Flags de compilação para otimização
CFLAGS="-O3 -march=native -flto -pthread"

echo "Compilando worker..."
cd /app/worker
gcc $CFLAGS -o worker worker.c -luuid

echo "Compilando banco de dados..."
cd /app/database
gcc $CFLAGS -o database database.c

echo "Verificando binários..."
if [ ! -f "/app/worker/worker" ] || [ ! -f "/app/database/database" ]; then
    echo "Erro: Falha na compilação"
    exit 1
fi

echo "Tornando scripts executáveis..."
chmod +x /app/src/*.sh
chmod +x /app/scripts/*.sh

echo "Compilação concluída com sucesso!"
