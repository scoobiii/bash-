#!/bin/bash

# =============================================================================
# test.sh - Script de teste para a Rinha de Backend 2023
# 
# Versão: 1.0.0
# Data: 28/04/2025
# 
# Responsabilidades:
# - Testar os endpoints da API
# - Verificar funcionalidades básicas
# - Validar regras de negócio
# 
# Equipe:
# - Zeh Sobrinho (Product Owner)
# - Manus (Full Stack AGI DevOp)
# =============================================================================

# Configurações
BASE_URL="http://localhost:9999"
TEST_RESULTS="test_results.txt"

# Cores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Função para executar um teste
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_status="$3"
    
    echo -e "${YELLOW}Executando teste: $test_name${NC}"
    
    # Executar comando e capturar status e resposta
    local response=$(eval "$command" 2>&1)
    local status=$(echo "$response" | grep -oP "HTTP/1.1 \K[0-9]+" || echo "000")
    
    # Verificar resultado
    if [ "$status" = "$expected_status" ]; then
        echo -e "${GREEN}✓ Teste passou! Status: $status${NC}"
        echo "=== $test_name ===" >> "$TEST_RESULTS"
        echo "Status: $status (esperado: $expected_status)" >> "$TEST_RESULTS"
        echo "Resposta: $response" >> "$TEST_RESULTS"
        echo "" >> "$TEST_RESULTS"
        return 0
    else
        echo -e "${RED}✗ Teste falhou! Status: $status (esperado: $expected_status)${NC}"
        echo "=== $test_name ===" >> "$TEST_RESULTS"
        echo "Status: $status (esperado: $expected_status)" >> "$TEST_RESULTS"
        echo "Resposta: $response" >> "$TEST_RESULTS"
        echo "" >> "$TEST_RESULTS"
        return 1
    fi
}

# Inicializar arquivo de resultados
echo "RESULTADOS DOS TESTES - $(date)" > "$TEST_RESULTS"
echo "============================" >> "$TEST_RESULTS"
echo "" >> "$TEST_RESULTS"

# Teste 1: Criar pessoa válida
echo "Teste 1: Criar pessoa válida"
PESSOA_VALIDA='{
    "nome": "John Doe",
    "apelido": "johndoe",
    "nascimento": "1990-01-01",
    "stack": ["Java", "Python", "JavaScript"]
}'

COMANDO_TESTE1="curl -v -X POST \"$BASE_URL/pessoas\" -H \"Content-Type: application/json\" -d '$PESSOA_VALIDA'"
run_test "Criar pessoa válida" "$COMANDO_TESTE1" "201"

# Extrair ID da pessoa criada (assumindo que está no header Location)
PESSOA_ID=$(curl -s -v -X POST "$BASE_URL/pessoas" -H "Content-Type: application/json" -d "$PESSOA_VALIDA" 2>&1 | grep -i "Location" | sed 's/.*\/pessoas\///')

# Teste 2: Buscar pessoa por ID
if [ -n "$PESSOA_ID" ]; then
    echo -e "\nTeste 2: Buscar pessoa por ID"
    COMANDO_TESTE2="curl -v \"$BASE_URL/pessoas/$PESSOA_ID\""
    run_test "Buscar pessoa por ID" "$COMANDO_TESTE2" "200"
else
    echo -e "${RED}✗ Não foi possível obter ID para teste de busca${NC}"
    echo "=== Buscar pessoa por ID ===" >> "$TEST_RESULTS"
    echo "Falha: Não foi possível obter ID para teste" >> "$TEST_RESULTS"
    echo "" >> "$TEST_RESULTS"
fi

# Teste 3: Buscar pessoa por termo
echo -e "\nTeste 3: Buscar pessoa por termo"
COMANDO_TESTE3="curl -v \"$BASE_URL/pessoas?t=John\""
run_test "Buscar pessoa por termo" "$COMANDO_TESTE3" "200"

# Teste 4: Contagem de pessoas
echo -e "\nTeste 4: Contagem de pessoas"
COMANDO_TESTE4="curl -v \"$BASE_URL/contagem-pessoas\""
run_test "Contagem de pessoas" "$COMANDO_TESTE4" "200"

# Teste 5: Criar pessoa inválida (sem nome)
echo -e "\nTeste 5: Criar pessoa inválida (sem nome)"
PESSOA_INVALIDA='{
    "apelido": "semNome",
    "nascimento": "1990-01-01",
    "stack": ["Java", "Python"]
}'

COMANDO_TESTE5="curl -v -X POST \"$BASE_URL/pessoas\" -H \"Content-Type: application/json\" -d '$PESSOA_INVALIDA'"
run_test "Criar pessoa inválida (sem nome)" "$COMANDO_TESTE5" "422"

# Teste 6: Buscar pessoa com ID inexistente
echo -e "\nTeste 6: Buscar pessoa com ID inexistente"
COMANDO_TESTE6="curl -v \"$BASE_URL/pessoas/00000000-0000-0000-0000-000000000000\""
run_test "Buscar pessoa com ID inexistente" "$COMANDO_TESTE6" "404"

# Teste 7: Método não permitido
echo -e "\nTeste 7: Método não permitido"
COMANDO_TESTE7="curl -v -X DELETE \"$BASE_URL/pessoas/$PESSOA_ID\""
run_test "Método não permitido" "$COMANDO_TESTE7" "405"

echo -e "\nTodos os testes foram executados. Resultados salvos em $TEST_RESULTS"
