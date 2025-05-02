/**
 * database.c - Implementação do componente de Banco de Dados da Rinha de Backend
 * 
 * Versão: 1.0.0
 * Data: 28/04/2025
 * 
 * Responsabilidades:
 * - Armazenar dados em memória
 * - Processar operações CRUD
 * - Implementar persistência via snapshots
 * 
 * Equipe:
 * - Zeh Sobrinho (Product Owner)
 * - Manus (Full Stack AGI DevOp)
 */

#include "database.h"
#include <stdarg.h>

// Variáveis globais
int server_fd;
Database global_db;

// Função para logging (simplificada)
void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// Função de hash (simples)
unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASH_TABLE_SIZE;
}

// Função para salvar snapshot dos dados
void save_snapshot(Database *db) {
    char filename[512];
    time_t now = time(NULL);
    sprintf(filename, "%s/snapshot_%ld.dat", db->data_dir, now);
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erro ao criar arquivo de snapshot");
        return;
    }
    
    pthread_mutex_lock(&db->mutex);
    
    // Salvar contador
    fwrite(&db->contador_pessoas, sizeof(int), 1, file);
    
    // Salvar dados
    for (int i = 0; i < MAX_PESSOAS; i++) {
        if (db->pessoas[i].usado) {
            fwrite(&i, sizeof(int), 1, file);
            fwrite(&db->pessoas[i], sizeof(Pessoa), 1, file);
        }
    }
    
    pthread_mutex_unlock(&db->mutex);
    
    fclose(file);
    log_message("Snapshot salvo em %s", filename);
    
    // Criar link simbólico para o snapshot mais recente
    char latest_link[512];
    sprintf(latest_link, "%s/latest_snapshot.dat", db->data_dir);
    unlink(latest_link);  // Remover link anterior se existir
    symlink(filename, latest_link);
}

// Função para carregar snapshot mais recente
void load_snapshot(Database *db) {
    char latest_link[512];
    sprintf(latest_link, "%s/latest_snapshot.dat", db->data_dir);
    
    FILE *file = fopen(latest_link, "rb");
    if (!file) {
        log_message("Nenhum snapshot encontrado para carregar");
        return;
    }
    
    // Carregar contador
    fread(&db->contador_pessoas, sizeof(int), 1, file);
    
    // Carregar dados e reconstruir hash tables
    int index;
    while (fread(&index, sizeof(int), 1, file) == 1) {
        if (index >= 0 && index < MAX_PESSOAS) {
            fread(&db->pessoas[index], sizeof(Pessoa), 1, file);
            db->pessoas[index].usado = 1; // Garantir que está marcado como usado
            
            // Adicionar ao hash table de ID
            unsigned int id_hash = hash_string(db->pessoas[index].id);
            db->pessoas[index].next = db->hash_table[id_hash];
            db->hash_table[id_hash] = index;
            
            // Adicionar ao hash table de apelido
            unsigned int apelido_hash = hash_string(db->pessoas[index].apelido);
            // TODO: Implementar hash table para apelido (requer estrutura diferente)
        } else {
            log_message("Erro: Índice inválido (%d) encontrado no snapshot", index);
            // Pular o registro inválido
            fseek(file, sizeof(Pessoa), SEEK_CUR);
        }
    }
    
    fclose(file);
    log_message("Snapshot carregado, %d pessoas recuperadas", db->contador_pessoas);
}

// Thread para salvar snapshots periodicamente
void *snapshot_thread(void *arg) {
    Database *db = (Database *)arg;
    while (1) {
        sleep(SNAPSHOT_INTERVAL);
        save_snapshot(db);
    }
    return NULL;
}

// Função para criar pessoa
int create_pessoa(Database *db, const Pessoa *pessoa) {
    pthread_mutex_lock(&db->mutex);
    
    // Verificar se apelido já existe (requer hash table de apelido)
    // TODO: Implementar verificação de apelido único
    
    // Verificar se há espaço
    if (db->contador_pessoas >= MAX_PESSOAS) {
        pthread_mutex_unlock(&db->mutex);
        return -1; // Limite atingido
    }
    
    // Encontrar slot livre (linear scan por enquanto)
    int index = -1;
    for (int i = 0; i < MAX_PESSOAS; i++) {
        if (!db->pessoas[i].usado) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        pthread_mutex_unlock(&db->mutex);
        return -2; // Erro interno, nenhum slot livre (não deveria acontecer)
    }
    
    // Copiar dados
    memcpy(&db->pessoas[index], pessoa, sizeof(Pessoa));
    db->pessoas[index].usado = 1;
    db->contador_pessoas++;
    
    // Adicionar ao hash table de ID
    unsigned int id_hash = hash_string(pessoa->id);
    db->pessoas[index].next = db->hash_table[id_hash];
    db->hash_table[id_hash] = index;
    
    // Adicionar ao hash table de apelido
    // TODO: Implementar hash table para apelido
    
    pthread_mutex_unlock(&db->mutex);
    return index; // Retorna o índice onde foi inserido
}

// Função para buscar pessoa por ID
int get_pessoa_by_id(Database *db, const char *id, Pessoa *pessoa_out) {
    pthread_mutex_lock(&db->mutex);
    
    unsigned int id_hash = hash_string(id);
    int current_index = db->hash_table[id_hash];
    int found = 0;
    
    while (current_index != -1) {
        if (db->pessoas[current_index].usado && strcmp(db->pessoas[current_index].id, id) == 0) {
            memcpy(pessoa_out, &db->pessoas[current_index], sizeof(Pessoa));
            found = 1;
            break;
        }
        current_index = db->pessoas[current_index].next;
    }
    
    pthread_mutex_unlock(&db->mutex);
    return found ? 0 : 1; // 0 se encontrado, 1 se não encontrado
}

// Função para buscar pessoas por termo
int search_pessoas(Database *db, const char *termo, Pessoa *resultado, int max_results) {
    pthread_mutex_lock(&db->mutex);
    
    int count = 0;
    // Busca linear por enquanto - Otimizar com índices (Trie, etc.)
    for (int i = 0; i < MAX_PESSOAS && count < max_results; i++) {
        if (db->pessoas[i].usado) {
            // Verificar termo em nome, apelido e stack
            if (strstr(db->pessoas[i].nome, termo) || 
                strstr(db->pessoas[i].apelido, termo) || 
                strstr(db->pessoas[i].stack, termo)) {
                
                memcpy(&resultado[count], &db->pessoas[i], sizeof(Pessoa));
                count++;
            }
        }
    }
    
    pthread_mutex_unlock(&db->mutex);
    return count;
}

// Função para contar pessoas
int count_pessoas(Database *db) {
    pthread_mutex_lock(&db->mutex);
    int count = db->contador_pessoas;
    pthread_mutex_unlock(&db->mutex);
    return count;
}

// Função para parsear comando (simplificada)
void parse_command(const char *buffer, char *operation, char *data) {
    char *delimiter = strchr(buffer, '|');
    if (delimiter) {
        strncpy(operation, buffer, delimiter - buffer);
        operation[delimiter - buffer] = '\0';
        strcpy(data, delimiter + 1);
    } else {
        strcpy(operation, buffer);
        data[0] = '\0';
    }
}

// Função para processar comandos do cliente
void process_command(int client_socket, Database *db) {
    char buffer[BUFFER_SIZE] = {0};
    char operation[50], data[BUFFER_SIZE];
    char response[BUFFER_SIZE * MAX_SEARCH_RESULTS]; // Buffer maior para busca
    
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';
    
    parse_command(buffer, operation, data);
    
    if (strcmp(operation, "CREATE") == 0) {
        Pessoa nova_pessoa;
        // Formato: id|nome|apelido|nascimento|stack
        char *token;
        char *rest = data;
        
        token = strtok_r(rest, "|", &rest); if (!token) goto cmd_error; strcpy(nova_pessoa.id, token);
        token = strtok_r(NULL, "|", &rest); if (!token) goto cmd_error; strcpy(nova_pessoa.nome, token);
        token = strtok_r(NULL, "|", &rest); if (!token) goto cmd_error; strcpy(nova_pessoa.apelido, token);
        token = strtok_r(NULL, "|", &rest); if (!token) goto cmd_error; strcpy(nova_pessoa.nascimento, token);
        token = strtok_r(NULL, "|", &rest); if (!token) goto cmd_error; strcpy(nova_pessoa.stack, token);
        
        int result = create_pessoa(db, &nova_pessoa);
        if (result >= 0) {
            snprintf(response, BUFFER_SIZE, "OK|%d", result);
        } else if (result == -1) {
            snprintf(response, BUFFER_SIZE, "ERROR|Limite de pessoas atingido");
        } else {
            snprintf(response, BUFFER_SIZE, "ERROR|Erro interno ao criar pessoa");
        }
        write(client_socket, response, strlen(response));
        
    } else if (strcmp(operation, "GET_BY_ID") == 0) {
        Pessoa pessoa_encontrada;
        int result = get_pessoa_by_id(db, data, &pessoa_encontrada);
        if (result == 0) {
            snprintf(response, BUFFER_SIZE, "OK|%s|%s|%s|%s|%s",
                     pessoa_encontrada.id,
                     pessoa_encontrada.nome,
                     pessoa_encontrada.apelido,
                     pessoa_encontrada.nascimento,
                     pessoa_encontrada.stack);
        } else {
            snprintf(response, BUFFER_SIZE, "NOT_FOUND|");
        }
        write(client_socket, response, strlen(response));
        
    } else if (strcmp(operation, "SEARCH") == 0) {
        Pessoa resultados[MAX_SEARCH_RESULTS];
        int count = search_pessoas(db, data, resultados, MAX_SEARCH_RESULTS);
        
        if (count >= 0) {
            int response_len = snprintf(response, sizeof(response), "OK|");
            for (int i = 0; i < count; i++) {
                response_len += snprintf(response + response_len, sizeof(response) - response_len,
                                         "%s|%s|%s|%s|%s|",
                                         resultados[i].id,
                                         resultados[i].nome,
                                         resultados[i].apelido,
                                         resultados[i].nascimento,
                                         resultados[i].stack);
                // Verificar se o buffer está cheio
                if (response_len >= sizeof(response) - BUFFER_SIZE) break; 
            }
            write(client_socket, response, response_len);
        } else {
            snprintf(response, BUFFER_SIZE, "ERROR|Erro ao buscar pessoas");
            write(client_socket, response, strlen(response));
        }
        
    } else if (strcmp(operation, "COUNT") == 0) {
        int count = count_pessoas(db);
        snprintf(response, BUFFER_SIZE, "OK|%d", count);
        write(client_socket, response, strlen(response));
        
    } else {
cmd_error:
        snprintf(response, BUFFER_SIZE, "ERROR|Comando desconhecido ou formato inválido");
        write(client_socket, response, strlen(response));
    }
    
    close(client_socket);
}

// Função para limpar recursos ao encerrar
void cleanup(int sig) {
    log_message("Encerrando banco de dados...");
    save_snapshot(&global_db);  // Salvar snapshot final
    close(server_fd);
    free(global_db.pessoas);
    free(global_db.hash_table);
    // free(global_db.apelido_hash); // Descomentar quando implementado
    pthread_mutex_destroy(&global_db.mutex);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <porta> <diretório_dados>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    strcpy(global_db.data_dir, argv[2]);
    
    // Registrar handler para sinais
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
    // Inicializar banco de dados
    global_db.pessoas = (Pessoa*)calloc(MAX_PESSOAS, sizeof(Pessoa));
    global_db.hash_table = (int*)malloc(HASH_TABLE_SIZE * sizeof(int));
    // global_db.apelido_hash = (int*)malloc(HASH_TABLE_SIZE * sizeof(int)); // Descomentar quando implementado
    if (!global_db.pessoas || !global_db.hash_table) {
        perror("Falha ao alocar memória para o banco");
        return 1;
    }
    // Inicializar hash tables com -1 (indicando vazio)
    memset(global_db.hash_table, -1, HASH_TABLE_SIZE * sizeof(int));
    // memset(global_db.apelido_hash, -1, HASH_TABLE_SIZE * sizeof(int)); // Descomentar quando implementado
    
    global_db.contador_pessoas = 0;
    pthread_mutex_init(&global_db.mutex, NULL);
    
    // Carregar snapshot se existir
    load_snapshot(&global_db);
    
    // Iniciar thread para salvar snapshots periodicamente
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, snapshot_thread, &global_db);
    pthread_detach(thread_id); // Não precisamos esperar por ela
    
    // Criar socket do servidor
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }
    
    // Configurar socket para reutilizar endereço/porta
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Vincular socket à porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    // Escutar por conexões
    if (listen(server_fd, 100) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    log_message("Banco de dados iniciado na porta %d, diretório de dados: %s", port, global_db.data_dir);
    
    // Loop principal para aceitar conexões
    while (1) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue; // Tentar aceitar próxima conexão
        }
        
        // Processar comando (em uma thread seria melhor para concorrência)
        // TODO: Implementar pool de threads para processar comandos
        process_command(client_socket, &global_db);
    }
    
    return 0; // Nunca deve chegar aqui
}

