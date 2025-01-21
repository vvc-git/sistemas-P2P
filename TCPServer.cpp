#include "TCPServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>

/**
 * @brief Construtor da classe TCPServer.
 */
TCPServer::TCPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager)
    : ip(ip), port(port), peer_id(peer_id), transfer_speed(transfer_speed), file_manager(file_manager) {
    
    // Cria um socket TCP IPv4 (SOCK_STREAM) especificando explicitamente o protocolo TCP (IPPROTO_TCP)
    // Nota: SOCK_STREAM já indica o uso de TCP, mas IPPROTO_TCP é passado para maior clareza e compatibilidade
    server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    // Prepara uma estrutura sockaddr_in para armazenar o endereço IP e a porta
    struct sockaddr_in my_addr = createSockAddr(ip.c_str(), port);

    // Associa o socket à porta e endereço IP especificados em my_addr
    if (bind(server_sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("Erro ao fazer bind no socket TCP");
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta para aceitar conexões de entrada
    // Constants::TCP_MAX_PENDING_CONNECTIONS define o número máximo de conexões pendentes na fila de espera
    if (listen(server_sockfd, Constants::TCP_MAX_PENDING_CONNECTIONS) < 0) {
        perror("Erro ao escutar no socket TCP");
        exit(EXIT_FAILURE);
    }

    logMessage(LogType::INFO, "Servidor TCP inicializado em " + ip + ":" + std::to_string(port));
}


/**
 * @brief Inicia o servidor TCP para aceitar conexões.
 */
void TCPServer::run() {
    while (true) {
        // Informações para o socket do cliente
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        // Aceita a conexão do cliente
        int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sockfd >= 0) {
            // Cria uma thread para lidar com o recebimento dos chunks
            std::thread(&TCPServer::receiveChunks, this, client_sockfd).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}


/**
 * @brief Recebe chunks enviados por um peer e ao receber todos, monta o arquivo final.
 */
void TCPServer::receiveChunks(int client_sockfd) {
    // Obtém o IP e a porta TCP do cliente
    auto [client_ip, client_port] = getClientAddressInfo(client_sockfd);
    
    // Continua a leitura até o cliente fechar a conexão
    while (true) {
        // Armazena a mensagem de controle recebida
        std::string control_message = "";

        // Quantidade de quantos bytes da mensagem de controle foram recebidos
        size_t control_message_total_bytes_received = 0;

        // Quantidade de bytes realmente recebido no recv
        ssize_t control_message_size = 0;

        // Buffer para armazenar os dados da mensagem de controle
        char control_message_buffer[Constants::CONTROL_MESSAGE_MAX_SIZE] = {0};

        // Recebe a mensagem de controle em pedaços
        do {
            // Recebe os dados
            control_message_size = recv(client_sockfd, control_message_buffer, Constants::CONTROL_MESSAGE_MAX_SIZE, 0);

            // Verifica se houve erro ou o cliente fechou a conexão
            if (control_message_size < 0) {
                perror("Erro ao receber a mensagem de controle");
                close(client_sockfd);
                return;
            } else if (control_message_size == 0) {
                logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                close(client_sockfd);
                return;
            }

            if (control_message_size > 0) {
                // Adiciona os bytes recebidos à mensagem de controle
                control_message.append(control_message_buffer, control_message_size);
                control_message_total_bytes_received += control_message_size;
            
                logMessage(LogType::INFO, "Recebido " + std::to_string(control_message_size) + " bytes da mensagem de controle de " + client_ip + ":" + std::to_string(client_port) + " (" + std::to_string(control_message_total_bytes_received) + "/" + std::to_string(Constants::CONTROL_MESSAGE_MAX_SIZE) + " bytes).");
            }

        } while (control_message_total_bytes_received < Constants::CONTROL_MESSAGE_MAX_SIZE); // Continua recebendo até que a mensagem de controle esteja completa

        logMessage(LogType::INFO, "Mensagem de controle '" + control_message + "' recebida de " + client_ip + ":" + std::to_string(client_port));
        
        // Transforma a string da mensagem de controle em um stream para extração
        std::stringstream control_message_stream(control_message);

        // Variáveis para armazenar os valores da mensagem de controle
        std::string command, file_name;
        int chunk_id, transfer_speed;
        size_t chunk_size;

        // Extrai os valores da mensagem de controle
        control_message_stream >> command >> file_name >> chunk_id >> transfer_speed >> chunk_size;

        // Verifica se o comando é "PUT", que indica recebimento de chunk de arquivo
        if (command == "PUT") {
            // Cria um buffer para armazenar o chunk completo
            char chunk_buffer[chunk_size] = {0};

            // Quantidade de quantos bytes do chunk foram recebidos
            size_t chunk_total_bytes_received = 0;

            // Continua recebendo o chunk até alcançar o tamanho esperado
            while (chunk_total_bytes_received < chunk_size) {
                // Quantidade de bytes realmente recebido no recv
                ssize_t chunk_bytes_received = 0;

                // Buffer temporário para armazenar os dados recebidos
                char chunk_temp_buffer[transfer_speed] = {0};

                // Recebe os dados do chunk
                chunk_bytes_received = recv(client_sockfd, chunk_temp_buffer, transfer_speed, 0);

                // Verifica se houve erro ou o cliente fechou a conexão
                if (chunk_bytes_received < 0) {
                    perror("Erro ao receber o chunk.");
                    close(client_sockfd);
                    return;
                } else if (chunk_bytes_received == 0) {
                    logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                    close(client_sockfd);
                    return;
                }

                if (chunk_bytes_received > 0) {
                    // Copia os dados recebidos para o buffer principal do chunk
                    std::memcpy(chunk_buffer + chunk_total_bytes_received, chunk_temp_buffer, chunk_bytes_received);

                    // Atualiza o total de bytes recebidos
                    chunk_total_bytes_received += chunk_bytes_received;

                    logMessage(LogType::CHUNK_RECEIVED, "Recebido " + std::to_string(chunk_bytes_received) + " bytes do chunk " + std::to_string(chunk_id) + " de " + client_ip + ":" + std::to_string(client_port) + " (" + std::to_string(chunk_total_bytes_received) + "/" + std::to_string(chunk_size) + " bytes).");
                }
            }

            // Verifica se todos os bytes esperados foram recebidos
            if (chunk_total_bytes_received >= chunk_size) {
                logMessage(LogType::SUCCESS, "SUCESSO AO RECEBER O CHUNK " + std::to_string(chunk_id) + " DO ARQUIVO " + file_name + " de " + client_ip + ":" + std::to_string(client_port));

                // Salva o chunk localmente
                file_manager.saveChunk(file_name, chunk_id, chunk_buffer, chunk_size);
            } else {
                logMessage(LogType::ERROR, "Falha ao receber o chunk " + std::to_string(chunk_id) + " de " + client_ip + ":" + std::to_string(client_port) + ". Bytes esperados: " + std::to_string(chunk_size) + ", recebidos: " + std::to_string(chunk_total_bytes_received));
            }
        }
    }

    // Fecha o socket após terminar
    close(client_sockfd);
}


/**
 * @brief Transfere chunks para o peer solicitante.
 */
void TCPServer::sendChunks(const std::string& file_name, const std::vector<int>& chunks, const PeerInfo& destination_info) {
    // Cria um novo socket para a conexão
    int new_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_sockfd < 0) {
        perror("Erro ao criar socket.");
        return;
    }

    // Estrutura para armazenar informações do endereço do destinatário
    struct sockaddr_in destination_addr = createSockAddr(destination_info.ip.c_str(), destination_info.port);

    // Tenta se conectar ao destinatário
    if (connect(new_sockfd, (struct sockaddr*)&destination_addr, sizeof(destination_addr)) < 0) {
        perror("Erro ao conectar ao peer.");
        close(new_sockfd);
        return;
    }

    // Itera sobre os chunks e envia um a um
    for (int chunk : chunks) {
        // Obtém o caminho do chunk
        std::string chunk_path = file_manager.getChunkPath(file_name, chunk);

        // Abre o arquivo em modo binário, somente leitura e posiciona o cursor no final para obter o tamanho
        std::ifstream chunk_file(chunk_path, std::ios::binary | std::ios::ate | std::ios::in);
        
        // Verifica se o arquivo foi encontrado/aberto
        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Chunk " + std::to_string(chunk) + " não encontrado.");
            continue;  // Pula para o próximo chunk
        }

        // Obtém o tamanho do chunk
        size_t chunk_size = chunk_file.tellg();

        // Volta para o início do arquivo
        chunk_file.seekg(0);
        
        // Cria um buffer em memória para armazenar todos os bytes do arquivo
        char file_buffer[chunk_size] = {0};
        
        // Lê o arquivo inteiro para o buffer
        chunk_file.read(file_buffer, chunk_size);

        // Fecha o arquivo após a leitura
        chunk_file.close();

        // Cria a mensagem de controle
        std::stringstream ss;
        ss << "PUT " << file_name << " " << chunk << " " << transfer_speed << " " << chunk_size;
        
        // transforma a stringstream em string
        std::string control_message = ss.str();

        // Define o buffer de controle com tamanho fixo e preenche com 0s
        char control_message_buffer[Constants::CONTROL_MESSAGE_MAX_SIZE] = {0};

        // Garante que a mensagem sempre vai ter no máximo o tamanho do buffer, - 1 para colocar o terminador de string
        int bytes_to_copy = std::min(control_message.size(), sizeof(control_message_buffer) - 1);

        // Copia a mensagem de controle para o buffer
        std::memcpy(control_message_buffer, control_message.c_str(), bytes_to_copy);

        // Adiciona um caractere nulo no final da mensagem para garantir o fim da string
        control_message_buffer[bytes_to_copy] = '\0';

        // Variável para armazenar o número total de bytes enviado
        size_t total_bytes_sent = 0;

        // Variável para armazenar o número de bytes a ser enviado
        size_t bytes_to_send = 0;

        // Variável para armazenar o número de bytes enviado no send
        size_t bytes_sent = 0;

        while (total_bytes_sent < Constants::CONTROL_MESSAGE_MAX_SIZE) {
            bytes_to_send = 0;
            bytes_sent = 0;

            // Calcula quantos bytes enviar no próximo bloco
            bytes_to_send = std::min(transfer_speed, Constants::CONTROL_MESSAGE_MAX_SIZE - static_cast<int>(total_bytes_sent));

            // Envia o bloco atual da mensagem
            bytes_sent = send(new_sockfd, control_message_buffer + total_bytes_sent, bytes_to_send, 0);
            
            // Verifica se houve erro ou o cliente fechou a conexão
            if (bytes_sent < 0) {
                perror("Erro ao enviar o bloco da mensagem de controle.");
                break;
            } else if (bytes_sent == 0) {
                logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                break;
            }

            total_bytes_sent += bytes_sent;

            logMessage(LogType::INFO, "Enviado " + std::to_string(bytes_sent) + " bytes da mensagem de controle para " + destination_info.ip + ":" + std::to_string(destination_info.port) + " (" + std::to_string(total_bytes_sent) + "/" + std::to_string(Constants::CONTROL_MESSAGE_MAX_SIZE) + " bytes).");

            // Simula a velocidade de transferência (bytes/segundo)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        total_bytes_sent = 0;;

        // Envia o chunk em blocos, respeitando a velocidade de transferência
        while (total_bytes_sent < chunk_size) {
            bytes_to_send = 0;
            bytes_sent = 0;

            // Calcula quantos bytes enviar no próximo bloco
            bytes_to_send = std::min(static_cast<size_t>(transfer_speed), chunk_size - total_bytes_sent);

            // Envia os bytes da estrutura em memória (file_buffer)
            bytes_sent = send(new_sockfd, file_buffer + total_bytes_sent, bytes_to_send, 0);

            // Verifica se houve erro ou o cliente fechou a conexão
            if (bytes_sent < 0) {
                perror("Erro ao enviar o chunk.");
                break;
            } else if (bytes_sent == 0) {
                logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                break;
            }

            total_bytes_sent += bytes_sent;

            logMessage(LogType::CHUNK_SENT, "Enviado " + std::to_string(bytes_sent) + " bytes do chunk " + std::to_string(chunk) + " do arquivo " + file_name + " para " + destination_info.ip + ":" + std::to_string(destination_info.port) + " (" + std::to_string(total_bytes_sent) + "/" + std::to_string(chunk_size) + " bytes).");

            // Simula a velocidade de transferência em bytes por segundo
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        logMessage(LogType::SUCCESS, "SUCESSO AO ENVIAR O CHUNK " + std::to_string(chunk) + " DO ARQUIVO " + file_name + " para " + destination_info.ip + ":" + std::to_string(destination_info.port));
    }

    // Fecha o socket após enviar todos os chunks
    close(new_sockfd);
}


/**
 * @brief Obtém o endereço IP e a porta TCP do cliente conectado via socket.
 */
std::tuple<std::string, int> TCPServer::getClientAddressInfo(int client_sockfd) {
    // Declara uma struct para armazenar o endereço do cliente
    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    // Obtém as informações do cliente conectado
    if (getpeername(client_sockfd, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        // Buffer para armazenar o endereço IP do cliente
        char client_ip[INET_ADDRSTRLEN];

        // Converte o IP para uma string
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        // Obtém a porta TCP do cliente
        int client_port = ntohs(client_addr.sin_port);

        // Retorna a tupla contendo o IP e a porta TCP
        return std::make_tuple(std::string(client_ip), client_port);
    } else {
        // Se houver um erro ao obter as informações, retorna uma tupla com valores padrão
        perror("Erro ao obter IP e porta TCP do cliente");
        return std::make_tuple("Erro", -1);
    }
}
