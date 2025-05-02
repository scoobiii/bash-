/**
 * worker.c - Implementação do componente Worker da Rinha de Backend
 * 
 * Versão: 1.0.1
 * Data: 28/04/2025
 * 
 * Responsabilidades:
 * - Processar requisições HTTP
 * - Implementar endpoints da API
 * - Comunicar com o banco de dados
 * 
 * Equipe:
 * - Zeh Sobrinho (Product Owner)
 * - Manus (Full Stack AGI DevOp)
 */

#include "worker.h"
#include <stdarg.h>

// Variáveis globais
int server_fd;
char db_address[100];

// Função para logging (simplificada)
void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// Função para conectar ao banco de dados
int connect_to_db(const char *db_address_param) {
    int sock;
    struct sockaddr_in db_addr;
    char host[64];
    int port;
    
    // Extrair host e porta do endereço do banco
    if (sscanf(db_address_param, "%[^:]:%d", host, &port) != 2) {
        log_message("Erro: Formato inválido do endereço do banco: %s", db_address_param);
        return -1;
    }
    
    // Criar socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    
    db_addr.sin_family = AF_INET;
    db_addr.sin_port = htons(port);
    
    // Converter endereço IP
    if (inet_pton(AF_INET, host, &db_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return -1;
    }
    
    // Conectar ao banco
    if (connect(sock, (struct sockaddr *)&db_addr, sizeof(db_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }
    
    log_message("Conectado ao banco de dados em %s:%d", host, port);
    return sock;
}

// Função para enviar comando e receber resposta do banco
int send_db_command(int db_socket, const char *command, char *response_buffer, size_t buffer_size) {
    if (write(db_socket, command, strlen(command)) < 0) {
        perror("Erro ao enviar comando para o banco");
        return -1;
    }
    
    ssize_t bytes_read = read(db_socket, response_buffer, buffer_size - 1);
    if (bytes_read <= 0) {
        perror("Erro ao receber resposta do banco ou conexão fechada");
        return -1;
    }
    
    response_buffer[bytes_read] = '\0'; // Null-terminate a resposta
    return 0;
}

// Função para criar pessoa no banco
int create_pessoa(int db_socket, const Pessoa *pessoa) {
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // Formato: CREATE|id|nome|apelido|nascimento|stack
    snprintf(command, BUFFER_SIZE, "CREATE|%s|%s|%s|%s|%s",
             pessoa->id,
             pessoa->nome,
             pessoa->apelido,
             pessoa->nascimento,
             pessoa->stack);
             
    if (send_db_command(db_socket, command, response, BUFFER_SIZE) != 0) {
        return -1;
    }
    
    // Verificar resposta (OK|index ou ERROR|mensagem)
    if (strncmp(response, "OK|", 3) == 0) {
        return 0; // Sucesso
    } else {
        log_message("Erro do banco ao criar pessoa: %s", response);
        return -1; // Erro
    }
}

// Função para buscar pessoa por ID no banco
int get_pessoa_by_id(int db_socket, const char *id, Pessoa *pessoa) {
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // Formato: GET_BY_ID|id
    snprintf(command, BUFFER_SIZE, "GET_BY_ID|%s", id);
    
    if (send_db_command(db_socket, command, response, BUFFER_SIZE) != 0) {
        return -1;
    }
    
    // Verificar resposta (OK|id|nome|apelido|nascimento|stack ou NOT_FOUND|)
    if (strncmp(response, "OK|", 3) == 0) {
        // Parsear os dados
        char *token;
        char *rest = response + 3; // Pular "OK|"
        
        token = strtok_r(rest, "|", &rest); if (!token) return -1; strcpy(pessoa->id, token);
        token = strtok_r(NULL, "|", &rest); if (!token) return -1; strcpy(pessoa->nome, token);
        token = strtok_r(NULL, "|", &rest); if (!token) return -1; strcpy(pessoa->apelido, token);
        token = strtok_r(NULL, "|", &rest); if (!token) return -1; strcpy(pessoa->nascimento, token);
        token = strtok_r(NULL, "|", &rest); if (!token) return -1; strcpy(pessoa->stack, token);
        
        return 0; // Encontrado
    } else if (strncmp(response, "NOT_FOUND|", 10) == 0) {
        return 1; // Não encontrado
    } else {
        log_message("Erro do banco ao buscar por ID: %s", response);
        return -1; // Erro
    }
}

// Função para buscar pessoas por termo no banco
int search_pessoas(int db_socket, const char *termo, Pessoa *pessoas_array, int max_results) {
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE * MAX_SEARCH_RESULTS]; // Buffer maior para múltiplos resultados
    
    // Formato: SEARCH|termo
    snprintf(command, BUFFER_SIZE, "SEARCH|%s", termo);
    
    if (send_db_command(db_socket, command, response, sizeof(response)) != 0) {
        return -1;
    }
    
    // Verificar resposta (OK|id1|nome1|...|idN|nomeN|...|)
    if (strncmp(response, "OK|", 3) == 0) {
        int count = 0;
        char *token;
        char *rest = response + 3; // Pular "OK|"
        
        while (count < max_results && (token = strtok_r(rest, "|", &rest)) != NULL) {
            strcpy(pessoas_array[count].id, token);
            token = strtok_r(NULL, "|", &rest); if (!token) break; strcpy(pessoas_array[count].nome, token);
            token = strtok_r(NULL, "|", &rest); if (!token) break; strcpy(pessoas_array[count].apelido, token);
            token = strtok_r(NULL, "|", &rest); if (!token) break; strcpy(pessoas_array[count].nascimento, token);
            token = strtok_r(NULL, "|", &rest); if (!token) break; strcpy(pessoas_array[count].stack, token);
            count++;
        }
        return count; // Retorna número de pessoas encontradas
    } else {
        log_message("Erro do banco ao buscar por termo: %s", response);
        return -1; // Erro
    }
}

// Função para contar pessoas no banco
int count_pessoas(int db_socket) {
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // Formato: COUNT|
    snprintf(command, BUFFER_SIZE, "COUNT|");
    
    if (send_db_command(db_socket, command, response, BUFFER_SIZE) != 0) {
        return -1;
    }
    
    // Verificar resposta (OK|count)
    if (strncmp(response, "OK|", 3) == 0) {
        return atoi(response + 3);
    } else {
        log_message("Erro do banco ao contar pessoas: %s", response);
        return -1; // Erro
    }
}

// Função para gerar UUID
void generate_uuid(char *uuid_str) {
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, uuid_str);
}

// Função para construir a resposta HTTP
void build_response(HttpResponse *response, int status_code, const char *content_type, const char *body) {
    response->status_code = status_code;
    strcpy(response->content_type, content_type);
    if (body) {
        strncpy(response->body, body, BUFFER_SIZE - 1);
        response->body[BUFFER_SIZE - 1] = '\0';
        response->body_length = strlen(response->body);
    } else {
        response->body[0] = '\0';
        response->body_length = 0;
    }
}

// Função para enviar a resposta HTTP ao cliente
void send_response(int client_socket, const HttpResponse *response) {
    char header[BUFFER_SIZE];
    const char *status_text;

    switch (response->status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 422: status_text = "Unprocessable Entity"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    snprintf(header, BUFFER_SIZE, 
             "HTTP/1.1 %d %s\r\n" 
             "Content-Type: %s\r\n" 
             "Content-Length: %zu\r\n" 
             "Connection: close\r\n\r\n", 
             response->status_code, status_text, 
             response->content_type, 
             response->body_length);

    // Ignorar retorno de write por enquanto, tratar erros de escrita seria ideal
    write(client_socket, header, strlen(header));
    if (response->body_length > 0) {
        write(client_socket, response->body, response->body_length);
    }
}

// Função para validar dados da pessoa (simplificada)
int validate_pessoa(const Pessoa *pessoa) {
    if (!pessoa->nome || strlen(pessoa->nome) == 0 || strlen(pessoa->nome) > 100) return 0;
    if (!pessoa->apelido || strlen(pessoa->apelido) == 0 || strlen(pessoa->apelido) > 32) return 0;
    if (!pessoa->nascimento || strlen(pessoa->nascimento) != 10) return 0; // Formato YYYY-MM-DD
    // TODO: Adicionar validação mais robusta para data e stack
    return 1;
}

// Função para parsear a requisição HTTP (simplificada)
void parse_request(const char *buffer, char *method, char *path, char *body) {
    // Extrair método e caminho
    sscanf(buffer, "%s %s", method, path);
    
    // Encontrar o início do corpo (após \r\n\r\n)
    char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start) {
        strcpy(body, body_start + 4);
    } else {
        body[0] = '\0';
    }
}

// Função auxiliar para extrair valor JSON string (robusta)
int extract_json_string(const char *json, const char *key, char *dest, size_t dest_size) {
    char search_key[100];
    snprintf(search_key, sizeof(search_key), "\"%s\":\"", key);
    
    char *start = strstr(json, search_key);
    if (!start) return 0; // Chave não encontrada
    
    start += strlen(search_key);
    char *end = strchr(start, '"');
    if (!end) return 0; // Aspa final não encontrada
    
    size_t len = end - start;
    if (len >= dest_size) len = dest_size - 1; // Evitar buffer overflow
    
    strncpy(dest, start, len);
    dest[len] = '\0';
    return 1;
}

// Função auxiliar para extrair valor JSON array (simplificada)
int extract_json_array_as_string(const char *json, const char *key, char *dest, size_t dest_size) {
    char search_key[100];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    char *start = strstr(json, search_key);
    if (!start) return 0; // Chave não encontrada
    
    start += strlen(search_key);
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++; // Pular espaços
    
    if (*start != '[') return 0; // Não é um array
    
    char *end = strchr(start, ']');
    if (!end) return 0; // Fim do array não encontrado
    
    size_t len = end - start + 1;
    if (len >= dest_size) len = dest_size - 1; // Evitar buffer overflow
    
    strncpy(dest, start, len);
    dest[len] = '\0';
    return 1;
}

// Função para processar uma requisição HTTP
void handle_request(int client_socket, int db_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char method[10], path[256], body[BUFFER_SIZE];
    HttpResponse http_response;
    
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';
    
    parse_request(buffer, method, path, body);
    
    if (strcmp(method, "POST") == 0 && strcmp(path, "/pessoas") == 0) {
        // Criar pessoa
        Pessoa nova_pessoa;
        memset(&nova_pessoa, 0, sizeof(Pessoa)); // Limpar estrutura

        // Extrair dados do JSON (usando funções auxiliares robustas)
        int nome_ok = extract_json_string(body, "nome", nova_pessoa.nome, sizeof(nova_pessoa.nome));
        int apelido_ok = extract_json_string(body, "apelido", nova_pessoa.apelido, sizeof(nova_pessoa.apelido));
        int nascimento_ok = extract_json_string(body, "nascimento", nova_pessoa.nascimento, sizeof(nova_pessoa.nascimento));
        int stack_ok = extract_json_array_as_string(body, "stack", nova_pessoa.stack, sizeof(nova_pessoa.stack));

        if (nome_ok && apelido_ok && nascimento_ok) { // Stack é opcional
            if (validate_pessoa(&nova_pessoa)) {
                generate_uuid(nova_pessoa.id);
                if (create_pessoa(db_socket, &nova_pessoa) == 0) {
                    char location_header[100];
                    snprintf(location_header, 100, "/pessoas/%s", nova_pessoa.id);
                    // Construir resposta 201 Created
                    // TODO: Adicionar header Location corretamente (modificar send_response)
                    build_response(&http_response, 201, "application/json", "{}");
                    send_response(client_socket, &http_response);
                } else {
                    build_response(&http_response, 500, "text/plain", "Erro ao criar pessoa no banco");
                    send_response(client_socket, &http_response);
                }
            } else {
                build_response(&http_response, 422, "text/plain", "Dados da pessoa inválidos");
                send_response(client_socket, &http_response);
            }
        } else {
            build_response(&http_response, 400, "text/plain", "Corpo da requisição inválido ou campos obrigatórios ausentes");
            send_response(client_socket, &http_response);
        }

    } else if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/pessoas/", 9) == 0) {
            // Busca por ID
            char id[37];
            strcpy(id, path + 9);
            Pessoa pessoa_encontrada;
            int result = get_pessoa_by_id(db_socket, id, &pessoa_encontrada);
            
            if (result == 0) {
                // Serializar pessoa para JSON
                char json_body[BUFFER_SIZE];
                snprintf(json_body, BUFFER_SIZE, 
                         "{\"id\":\"%s\",\"nome\":\"%s\",\"apelido\":\"%s\",\"nascimento\":\"%s\",\"stack\":%s}",
                         pessoa_encontrada.id, pessoa_encontrada.nome, pessoa_encontrada.apelido, 
                         pessoa_encontrada.nascimento, 
                         (strlen(pessoa_encontrada.stack) > 0 ? pessoa_encontrada.stack : "null")); // Tratar stack vazio
                build_response(&http_response, 200, "application/json", json_body);
                send_response(client_socket, &http_response);
            } else if (result == 1) {
                build_response(&http_response, 404, "text/plain", "Pessoa não encontrada");
                send_response(client_socket, &http_response);
            } else {
                build_response(&http_response, 500, "text/plain", "Erro ao buscar pessoa no banco");
                send_response(client_socket, &http_response);
            }
        } else if (strncmp(path, "/pessoas?t=", 11) == 0) {
            // Busca por termo
            char termo[100];
            strcpy(termo, path + 11); // TODO: Decodificar URL encoding
            Pessoa resultados[MAX_SEARCH_RESULTS];
            int count = search_pessoas(db_socket, termo, resultados, MAX_SEARCH_RESULTS);
            
            if (count >= 0) {
                // Serializar lista de pessoas para JSON
                char json_body[BUFFER_SIZE * MAX_SEARCH_RESULTS] = "[";
                int current_len = 1;
                for (int i = 0; i < count; i++) {
                    char pessoa_json[BUFFER_SIZE];
                    int pessoa_len = snprintf(pessoa_json, BUFFER_SIZE, 
                             "%s{\"id\":\"%s\",\"nome\":\"%s\",\"apelido\":\"%s\",\"nascimento\":\"%s\",\"stack\":%s}",
                             (i > 0 ? "," : ""), // Adicionar vírgula antes do segundo item em diante
                             resultados[i].id, resultados[i].nome, resultados[i].apelido, 
                             resultados[i].nascimento, 
                             (strlen(resultados[i].stack) > 0 ? resultados[i].stack : "null")); // Tratar stack vazio
                    
                    // Verificar se cabe no buffer
                    if (current_len + pessoa_len < sizeof(json_body) - 1) {
                        strcat(json_body, pessoa_json);
                        current_len += pessoa_len;
                    } else {
                        // Buffer cheio, parar de adicionar
                        break;
                    }
                }
                strcat(json_body, "]");
                build_response(&http_response, 200, "application/json", json_body);
                send_response(client_socket, &http_response);
            } else {
                build_response(&http_response, 500, "text/plain", "Erro ao buscar pessoas no banco");
                send_response(client_socket, &http_response);
            }
        } else if (strcmp(path, "/contagem-pessoas") == 0) {
            // Contagem de pessoas
            int count = count_pessoas(db_socket);
            if (count >= 0) {
                char count_str[20];
                snprintf(count_str, 20, "%d", count);
                build_response(&http_response, 200, "text/plain", count_str);
                send_response(client_socket, &http_response);
            } else {
                build_response(&http_response, 500, "text/plain", "Erro ao contar pessoas no banco");
                send_response(client_socket, &http_response);
            }
        } else {
            build_response(&http_response, 404, "text/plain", "Rota não encontrada");
            send_response(client_socket, &http_response);
        }
    } else {
        build_response(&http_response, 405, "text/plain", "Método não permitido");
        send_response(client_socket, &http_response);
    }
    
    close(client_socket);
}

// Função para limpar recursos ao encerrar
void cleanup(int sig) {
    log_message("Encerrando worker...");
    close(server_fd);
    // Não fechar db_socket aqui, pois pode ser compartilhado ou gerenciado externamente
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <porta> <endereço_banco>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    strcpy(db_address, argv[2]);
    
    // Registrar handler para sinais
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
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
    if (listen(server_fd, BACKLOG_SIZE) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    log_message("Worker iniciado na porta %d, pronto para aceitar conexões", port);
    
    // Loop principal para aceitar conexões
    while (1) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue; // Tentar aceitar próxima conexão
        }
        
        // Conectar ao banco de dados para esta requisição (ou reusar conexão se implementado pooling)
        int db_socket = connect_to_db(db_address);
        if (db_socket < 0) {
            log_message("Falha ao conectar ao banco para processar requisição");
            HttpResponse err_response;
            build_response(&err_response, 500, "text/plain", "Erro interno do servidor");
            send_response(client_socket, &err_response);
            close(client_socket);
            continue;
        }
        
        // Processar requisição
        handle_request(client_socket, db_socket);
        
        // Fechar conexão com o banco após processar a requisição
        close(db_socket);
    }
    
    return 0; // Nunca deve chegar aqui no loop infinito
}

