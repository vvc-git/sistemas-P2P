#ifndef UTILS_H
#define UTILS_H

#include "Constants.h"
#include <iostream>
#include <regex>
#include <string>


/**
 * @brief Enumeração para os tipos de mensagens de log
 */
enum class LogType {
    ERROR,
    INFO,
    DISCOVERY_RECEIVED,
    DISCOVERY_SENT,
    REQUEST_RECEIVED,
    REQUEST_SENT,
    RESPONSE_RECEIVED,
    RESPONSE_SENT,
    CHUNK_SENT,
    CHUNK_RECEIVED,
    SUCCESS,
    OTHER
};


/**
 * @brief Remove espaços em branco ao redor de uma string.
 * 
 * @param str String que deseja remover os espaços em branco ao redor.
 * @return A string sem espaços em branco ao redor.
 */
std::string trim(const std::string& str);


/**
 * @brief Formata e exibe mensagens de log de forma consistente, com cores.
 * 
 * @param type Tipo da mensagem.
 * @param message A mensagem a ser exibida no log.
 */
void logMessage(LogType type, const std::string& message);

/**
 * @brief Exibe uma mensagem de sucesso.
 * 
 * Essa função imprime uma mensagem personalizada no terminal.
 * 
 * @param file_name Nome do arquivo que o peer montou com sucesso.
 * @param peer_id Id do Peer que está imprimindo a mensagem de sucesso.
 */
void displaySuccessMessage(const std::string& file_name, const std::string& peer_id);

/**
 * @brief Cria e configura uma estrutura sockaddr_in com base no IP e na porta fornecidos.
 * 
 * @param ip Endereço IP a ser configurado.
 * @param port Porta a ser configurada.
 * @return struct sockaddr_in configurada.
 */
struct sockaddr_in createSockAddr(const std::string& ip, int port);

#endif // UTILS_H
