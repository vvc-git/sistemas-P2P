#include "Utils.h"
#include <mutex>
#include <arpa/inet.h>
#include <cstring>


//< Mutex para proteger a saída do console
std::mutex cout_mutex;


/**
 * @brief Remove espaços em branco ao redor de uma string.
 */
std::string trim(const std::string& str) {
    return std::regex_replace(str, std::regex("^\\s+|\\s+$"), "");
}


/**
 * @brief Formata e exibe mensagens de log de forma consistente, com cores.
 */
void logMessage(LogType type, const std::string& message) {
    std::lock_guard<std::mutex> message_lock(cout_mutex); // Bloqueia o acesso à saída do console
    switch (type) {
        case LogType::DISCOVERY_RECEIVED:
            std::cout << Constants::YELLOW << "[DISCOVERY_RECEIVED] " << message;
            break;
        case LogType::DISCOVERY_SENT:
            std::cout << Constants::MAGENTA << "[DISCOVERY_SENT] " << message;
            break;
        case LogType::RESPONSE_RECEIVED:
            std::cout << Constants::CYAN << "[RESPONSE_RECEIVED] " << message;
            break;
        case LogType::RESPONSE_SENT:
            std::cout << Constants::GRAY << "[RESPONSE_SENT] " << message;
            break;
        case LogType::REQUEST_RECEIVED:
            std::cout << Constants::ORANGE << "[REQUEST_RECEIVED] " << message;
            break;
        case LogType::REQUEST_SENT:
            std::cout << Constants::PINK << "[REQUEST_SENT] " << message;
            break;
        case LogType::CHUNK_RECEIVED:
            std::cout << Constants::GOLD << "[CHUNK_RECEIVED] " << message;
            break;
        case LogType::CHUNK_SENT:
            std::cout << Constants::AQUA << "[CHUNK_SENT] " << message;
            break;
        case LogType::SUCCESS:
            std::cout << Constants::GREEN << "[SUCCESS] " << message;
            break;
        case LogType::INFO:
            std::cout << Constants::BLUE << "[INFO] " << message;
            break;
        case LogType::ERROR:
            std::cout << Constants::RED << "[ERROR] " << message;
            break;
        default:
            std::cout << Constants::ORANGE << "[OTHER] " << message;
            break;
    }
    std::cout << Constants::RESET << std::endl; // Reseta a cor do texto e finaliza a linha
}


/**
 * @brief Exibe uma mensagem de sucesso.
 */
void displaySuccessMessage(const std::string& file_name, const std::string& peer_id) {
    std::lock_guard<std::mutex> message_lock(cout_mutex); // Bloqueia o acesso à saída do console
    
    // Definição das cores
    std::string colors[] = {
        Constants::RED,
        Constants::YELLOW,
        Constants::GREEN,
        Constants::BLUE,
        Constants::MAGENTA,
        Constants::RED,
    };

    // Mensagem central em branco
    std::string message = "Arquivo " + file_name + " montado com sucesso no Peer " + peer_id + "!";
    int width = message.length() + 8;

    // Exibe as bordas coloridas (superior)
    for (int i = 0; i < 3; ++i) {
        std::cout << colors[i] << std::string(width, '#') << Constants::RESET << "\n";
    }

    // Moldura interna (superior)
    std::cout << colors[3] << "###" << colors[4] << std::string(width - 6, ' ') << colors[3] << "###" << Constants::RESET << "\n";

    // Mensagem central em branco
    std::cout << colors[3] << "### " << Constants::RESET << message << colors[3] << " ###" << Constants::RESET << "\n";

    // Moldura interna (inferior)
    std::cout << colors[3] << "###" << colors[4] << std::string(width - 6, ' ') << colors[3] << "###" << Constants::RESET << "\n";

    // Exibe as bordas coloridas (inferior)
    for (int i = 0; i < 3; ++i) {
        std::cout << colors[i] << std::string(width, '#') << Constants::RESET << "\n";
    }
}


/**
 * @brief Cria e configura uma estrutura sockaddr_in com base no IP e na porta fornecidos.
 */
struct sockaddr_in createSockAddr(const std::string& ip, int port) {
    // Estrutura para armazenar informações do endereço do socket
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET; // Define a família de endereços como IPv4
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // Converte o endereço IP da string para o formato de rede
    addr.sin_port = htons(port); // Converte a porta para o formato de rede

    return addr; // Retorna a estrutura configurada
}
