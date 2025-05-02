#!/bin/bash

# =============================================================================
# orchestrator.sh - Orquestrador principal para a Rinha de Backend 2023
# 
# Versão: 1.0.0
# Data: 28/04/2025
# 
# Responsabilidades:
# - Iniciar e gerenciar todos os componentes do sistema
# - Monitorar a saúde dos processos
# - Implementar recuperação automática de falhas
# - Gerenciar ciclo de vida da aplicação
# 
# Equipe:
# - Zeh Sobrinho (Product Owner)
# - Manus (Full Stack AGI DevOp)
# =============================================================================

# Importar funções utilitárias
source "/app/src/utils.sh"

# Configurações
NUM_WORKERS=2
WORKER_BASE_PORT=8081
DB_PORT=7000
DATA_DIR="/app/data"
LOGS_DIR="/app/logs"

# Arrays para armazenar PIDs
WORKER_PIDS=()
DB_PID=""

# Função para iniciar o banco de dados
start_database() {
    log_info "Iniciando o banco de dados em memória na porta $DB_PORT..."
    /app/database/database "$DB_PORT" "$DATA_DIR" > "$LOGS_DIR/database.log" 2>&1 &
    DB_PID=$!
    
    # Verificar se o processo iniciou
    if ! is_process_running "$DB_PID"; then
        log_error "Falha ao iniciar o banco de dados"
        return 1
    fi
    
    log_info "Banco de dados iniciado com PID $DB_PID"
    
    # Aguardar o banco de dados iniciar e estar pronto
    local max_attempts=10
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if check_db_health "$DB_PORT"; then
            log_info "Banco de dados está pronto para receber conexões"
            return 0
        fi
        
        log_info "Aguardando banco de dados iniciar (tentativa $attempt/$max_attempts)..."
        sleep 1
        ((attempt++))
    done
    
    log_error "Banco de dados não respondeu após $max_attempts tentativas"
    return 1
}

# Função para iniciar um worker
start_worker() {
    local worker_id="$1"
    local worker_port=$((WORKER_BASE_PORT + worker_id))
    
    log_info "Iniciando worker #$worker_id na porta $worker_port..."
    /app/worker/worker "$worker_port" "127.0.0.1:$DB_PORT" > "$LOGS_DIR/worker_${worker_id}.log" 2>&1 &
    local pid=$!
    
    # Verificar se o processo iniciou
    if ! is_process_running "$pid"; then
        log_error "Falha ao iniciar worker #$worker_id"
        return 1
    fi
    
    log_info "Worker #$worker_id iniciado com PID $pid"
    WORKER_PIDS[$worker_id]=$pid
    
    # Aguardar o worker iniciar e estar pronto
    local max_attempts=5
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if check_worker_health "$worker_port"; then
            log_info "Worker #$worker_id está pronto para receber requisições"
            return 0
        fi
        
        log_info "Aguardando worker #$worker_id iniciar (tentativa $attempt/$max_attempts)..."
        sleep 1
        ((attempt++))
    done
    
    log_error "Worker #$worker_id não respondeu após $max_attempts tentativas"
    return 1
}

# Função para iniciar todos os workers
start_all_workers() {
    log_info "Iniciando $NUM_WORKERS workers..."
    
    local success=true
    for ((i=0; i<NUM_WORKERS; i++)); do
        if ! start_worker "$i"; then
            success=false
        fi
    done
    
    if $success; then
        log_info "Todos os workers foram iniciados com sucesso"
        return 0
    else
        log_error "Alguns workers falharam ao iniciar"
        return 1
    fi
}

# Função para monitorar e recuperar componentes
monitor_components() {
    log_info "Iniciando monitoramento de componentes..."
    
    while true; do
        # Verificar banco de dados
        if ! is_process_running "$DB_PID" || ! check_db_health "$DB_PORT"; then
            log_warning "Banco de dados não está respondendo, tentando reiniciar..."
            kill -TERM "$DB_PID" 2>/dev/null
            start_database
        else
            # Registrar uso de recursos
            local db_memory=$(check_memory_usage "$DB_PID")
            local db_cpu=$(check_cpu_usage "$DB_PID")
            log_info "Banco de dados: Memória=${db_memory}KB, CPU=${db_cpu}%"
        fi
        
        # Verificar workers
        for ((i=0; i<NUM_WORKERS; i++)); do
            local worker_pid="${WORKER_PIDS[$i]}"
            local worker_port=$((WORKER_BASE_PORT + i))
            
            if ! is_process_running "$worker_pid" || ! check_worker_health "$worker_port"; then
                log_warning "Worker #$i não está respondendo, tentando reiniciar..."
                kill -TERM "$worker_pid" 2>/dev/null
                start_worker "$i"
            else
                # Registrar uso de recursos
                local worker_memory=$(check_memory_usage "$worker_pid")
                local worker_cpu=$(check_cpu_usage "$worker_pid")
                log_info "Worker #$i: Memória=${worker_memory}KB, CPU=${worker_cpu}%"
            fi
        done
        
        sleep "$HEALTH_CHECK_INTERVAL"
    done
}

# Função para encerrar todos os processos ao receber um sinal
cleanup() {
    log_info "Recebido sinal para encerrar, limpando recursos..."
    cleanup_resources "$DB_PID" "${WORKER_PIDS[@]}"
    exit 0
}

# Função principal
main() {
    log_info "Iniciando orquestrador da Rinha de Backend 2023..."
    
    # Garantir que os diretórios necessários existem
    ensure_directories
    
    # Registrar handlers para sinais
    trap cleanup SIGINT SIGTERM
    
    # Iniciar componentes
    if ! start_database; then
        log_error "Falha ao iniciar o banco de dados, encerrando..."
        exit 1
    fi
    
    if ! start_all_workers; then
        log_error "Falha ao iniciar workers, encerrando..."
        cleanup_resources "$DB_PID"
        exit 1
    fi
    
    log_info "Sistema iniciado e pronto para receber requisições!"
    
    # Iniciar monitoramento em background
    monitor_components &
    MONITOR_PID=$!
    
    # Aguardar sinais
    wait
}

# Iniciar o orquestrador
main
