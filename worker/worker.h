/**
 * worker.h - Definições para o componente Worker da Rinha de Backend
 * 
 * Versão: 1.0.1
 * Data: 28/04/2025
 * 
 * Responsabilidades:
 * - Definir estruturas de dados para o worker
 * - Declarar funções para processamento HTTP
 * - Configurar constantes e parâmetros
 * 
 * Equipe:
 * - Zeh Sobrinho (Product Owner)
 * - Manus (Full Stack AGI DevOp)
 */

#ifndef WORKER_H
#define WORKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <uuid/uuid.h>

// Constantes
#define BUFFER_SIZE 8192
#define MAX_BATCH_SIZE 100
#define MAX_CONNECTIONS 1000
#define BACKLOG_SIZE 100
#define MAX_SEARCH_RESULTS 50 // Definido aqui

// Estrutura para armazenar uma pessoa
typedef struct {
    char id[37];  // UUID
    char nome[101];
    char apelido[33];
    char nascimento[11];  // YYYY-MM-DD
    char stack[1024];  // Lista de tecnologias
} Pessoa;

// Estrutura para resposta HTTP
typedef struct {
    int status_code;
    char content_type[50];
    char body[BUFFER_SIZE];
    size_t body_length;
} HttpResponse;

// Funções para comunicação com o banco de dados
int connect_to_db(const char *db_address);
int create_pessoa(int db_socket, const Pessoa *pessoa);
int get_pessoa_by_id(int db_socket, const char *id, Pessoa *pessoa);
int search_pessoas(int db_socket, const char *termo, Pessoa *pessoas, int max_results);
int count_pessoas(int db_socket);

// Funções para processamento HTTP
void handle_request(int client_socket, int db_socket);
void parse_request(const char *buffer, char *method, char *path, char *body);
int validate_pessoa(const Pessoa *pessoa);
void generate_uuid(char *uuid_str);
void build_response(HttpResponse *response, int status_code, const char *content_type, const char *body);
void send_response(int client_socket, const HttpResponse *response);

// Funções de utilidade
void cleanup(int sig);
void log_message(const char *format, ...);

#endif // WORKER_H
