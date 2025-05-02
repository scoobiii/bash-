# Rinha de Backend 2023 - Bash como Orquestrador

**Versão:** 1.0.0  
**Data:** 28/04/2025  
**Equipe:**  
- Zeh Sobrinho (Product Owner)  
- Manus (Full Stack AGI DevOp)

## Visão Geral

Uma implementação de alta performance para a [Rinha de Backend 2023 Q3](https://github.com/zanfranceschi/rinha-de-backend-2023-q3) utilizando Bash como orquestrador de componentes nativos em C.

## Arquitetura

Esta solução utiliza uma arquitetura distribuída inspirada em princípios de microsserviços e sistemas de alta disponibilidade:

```ascii
                   ┌─────────────┐
                   │    Nginx    │
                   │ Balanceador │
                   └──────┬──────┘
                          │
                 ┌────────┴────────┐
                 │                 │
          ┌──────▼─────┐    ┌──────▼─────┐
          │  Worker C  │    │  Worker C  │
          │ (Porta 8081)│    │ (Porta 8082)│
          └──────┬─────┘    └──────┬─────┘
                 │                 │
                 └────────┬────────┘
                          │
                    ┌─────▼─────┐
                    │ Banco de  │
                    │ Dados C   │
                    │ In-Memory │
                    └───────────┘
```

### Componentes

1. **Bash Orchestrator (`src/orchestrator.sh`)**
   - Responsável por iniciar, monitorar e gerenciar todos os componentes
   - Implementa recuperação automática de falhas (auto-healing)
   - Gerencia ciclo de vida dos processos

2. **Nginx Load Balancer (`nginx/nginx.conf`)**
   - Distribui requisições entre múltiplos workers
   - Configurado para máxima eficiência com número reduzido de workers
   - Implementa keep-alive para reduzir overhead de conexão

3. **C Workers (`worker/worker.c`)**
   - Servidores HTTP leves implementados em C puro
   - Processam requisições REST e implementam a lógica de negócio
   - Comunicam-se com o banco de dados via protocolo TCP personalizado

4. **In-Memory Database (`database/database.c`)**
   - Banco de dados em memória implementado em C
   - Utiliza estruturas de dados otimizadas para busca e inserção
   - Implementa persistência via snapshots periódicos
   - Suporta recuperação de dados após reinicialização

### Design Patterns Utilizados

1. **Padrão Arquitetural**
   - **Microservices:** Componentes independentes com responsabilidades únicas
   - **Event-Driven:** Comunicação assíncrona entre componentes

2. **Padrões de Design**
   - **Singleton:** Banco de dados como instância única
   - **Factory Method:** Criação de objetos Pessoa
   - **Observer:** Monitoramento de processos pelo orquestrador
   - **Command:** Protocolo de comunicação entre worker e banco

3. **Padrões DevOps**
   - **Infrastructure as Code:** Configuração completa via Docker Compose
   - **Continuous Testing:** Scripts de teste automatizados
   - **Health Checks:** Monitoramento contínuo de componentes

### Princípios XP (Extreme Programming) Aplicados

1. **Simplicidade:** Código mínimo necessário para resolver o problema
2. **Feedback Rápido:** Testes automatizados para validação imediata
3. **Iterações Curtas:** Desenvolvimento incremental com melhorias constantes
4. **Refatoração Contínua:** Melhoria constante do código sem alterar comportamento
5. **Integração Contínua:** Testes automatizados a cada alteração

## Estrutura de Diretórios

```
rinha-bash-orquestrador/
├── Dockerfile                # Configuração de build da imagem Docker
├── docker-compose.yml        # Configuração dos serviços
├── README.md                 # Documentação do projeto
├── database/                 # Componente de banco de dados
│   ├── database.c            # Implementação do banco em memória
│   └── database.h            # Definições e estruturas do banco
├── nginx/                    # Configuração do balanceador
│   └── nginx.conf            # Configuração otimizada do Nginx
├── scripts/                  # Scripts auxiliares
│   ├── build.sh              # Script de compilação
│   ├── test.sh               # Script de testes básicos
│   └── benchmark.sh          # Script de benchmark
├── src/                      # Scripts de orquestração
│   ├── orchestrator.sh       # Orquestrador principal
│   └── utils.sh              # Funções utilitárias
└── worker/                   # Componente de processamento
    ├── worker.c              # Implementação do servidor HTTP
    ├── worker.h              # Definições e estruturas do worker
    ├── http_parser.c         # Parser HTTP otimizado
    └── http_parser.h         # Definições do parser
```

## Limitações de Recursos

Esta implementação respeita os limites da Rinha de Backend:
- 1,5 CPU total
- 3GB RAM total

## Estratégias de Otimização

1. **Redução de Overhead de Rede**
   - Uso de `network_mode: host` para comunicação direta
   - Protocolo de comunicação binário entre worker e banco

2. **Otimização de Memória**
   - Alocação estática para evitar fragmentação
   - Estruturas de dados compactas
   - Reuso de buffers para evitar realocações

3. **Otimização de CPU**
   - Algoritmos de busca otimizados (hash tables)
   - Processamento em lote (batch) para inserções
   - Compilação com flags de otimização (-O3)

4. **Persistência Eficiente**
   - Snapshots incrementais em background
   - Journaling para operações críticas

5. **Balanceamento de Carga**
   - Configuração otimizada do Nginx
   - Número de workers ajustado para máxima eficiência

## Como Executar

```bash
# Clonar o repositório
git clone https://github.com/seu-usuario/rinha-bash-orquestrador.git
cd rinha-bash-orquestrador

# Construir e iniciar os serviços
docker-compose up -d

# Verificar logs
docker-compose logs -f

# Executar testes básicos
bash scripts/test.sh

# Executar benchmark
bash scripts/benchmark.sh
```

## Resultados de Performance

*Nota: Os resultados serão atualizados após os testes de benchmark.*

## Licença

MIT

