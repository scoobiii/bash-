#!/bin/bash

# =============================================================================
# utils.sh - Funções utilitárias para o orquestrador da Rinha de Backend
# 
# Versão: 1.0.0
# Data: 28/04/2025
# 
# Responsabilidades:
# - Fornecer funções auxiliares para o orquestrador
# - Implementar logging e monitoramento
# - Gerenciar verificações de saúde dos componentes
# 
# Equipe:
# - Zeh Sobrinho (Product Owner)
# - Manus (Full Stack AGI DevOp)
# =============================================================================

# Configurações
LOG_FILE="/app/logs/orchestrator.log"
HEALTH_CHECK_INTERVAL=5  # segundos

# Função para logging
log() {
    local level="$1"
    local message="$2"
    local timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_FILE"
}

# Função para logging de informações
log_info() {
    log "INFO" "$1"
}

# Função para logging de erros
log_error() {
    log "ERROR" "$1"
}

# Função para logging de avisos
log_warning() {
    log "WARNING" "$1"
}

# Função para verificar se um processo está rodando
is_process_running() {
    local pid="$1"
    if [ -z "$pid" ]; then
        return 1
    fi
    
    if kill -0 "$pid" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Função para verificar a saúde de um worker
check_worker_health() {
    local port="$1"
    local timeout=2
    
    # Tentar conectar ao worker
    if timeout "$timeout" bash -c "echo > /dev/tcp/127.0.0.1/$port" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Função para verificar a saúde do banco de dados
check_db_health() {
    local port="$1"
    local timeout=2
    
    # Tentar conectar ao banco
    if timeout "$timeout" bash -c "echo > /dev/tcp/127.0.0.1/$port" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Função para verificar uso de memória
check_memory_usage() {
    local pid="$1"
    local memory_kb=$(ps -o rss= -p "$pid" 2>/dev/null)
    
    if [ -z "$memory_kb" ]; then
        echo "0"
    else
        echo "$memory_kb"
    fi
}

# Função para verificar uso de CPU
check_cpu_usage() {
    local pid="$1"
    local cpu=$(ps -o %cpu= -p "$pid" 2>/dev/null)
    
    if [ -z "$cpu" ]; then
        echo "0.0"
    else
        echo "$cpu"
    fi
}

# Função para criar diretórios necessários
ensure_directories() {
    mkdir -p "/app/data"
    mkdir -p "/app/logs"
    
    # Verificar se os diretórios foram criados com sucesso
    if [ ! -d "/app/data" ] || [ ! -d "/app/logs" ]; then
        log_error "Falha ao criar diretórios necessários"
        return 1
    fi
    
    return 0
}

# Função para limpar recursos ao encerrar
cleanup_resources() {
    log_info "Encerrando recursos e processos..."
    
    # Lista de PIDs a encerrar
    local pids=("$@")
    
    for pid in "${pids[@]}"; do
        if is_process_running "$pid"; then
            log_info "Encerrando processo $pid"
            kill -TERM "$pid" 2>/dev/null
            
            # Aguardar até 5 segundos pelo encerramento
            for i in {1..5}; do
                if ! is_process_running "$pid"; then
                    break
                fi
                sleep 1
            done
            
            # Se ainda estiver rodando, forçar encerramento
            if is_process_running "$pid"; then
                log_warning "Forçando encerramento do processo $pid"
                kill -KILL "$pid" 2>/dev/null
            fi
        fi
    done
    
    log_info "Todos os recursos foram liberados"
}

