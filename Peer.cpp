#include "Peer.h"
#include <thread>
#include <iostream>
#include <fstream>

/**
 * @brief Construtor da classe Peer. Também inicializa os servidores UDP e TCP e o gerenciador de arquivos.
 */
Peer::Peer(int id, const std::string& ip, int udp_port, int tcp_port, int transfer_speed, const std::vector<std::tuple<std::string, int>> neighbors)
    : id(id), ip(ip), udp_port(udp_port), tcp_port(tcp_port), transfer_speed(transfer_speed), neighbors(neighbors),
      file_manager(std::to_string(id)),
      tcp_server(ip, tcp_port, id, transfer_speed, file_manager),
      udp_server(ip, udp_port, tcp_port, id, transfer_speed, file_manager, tcp_server) {}


/**
 * @brief Inicia os servidores TCP e UDP.
 */
void Peer::start(const std::vector<std::string>& file_names) {
    // Inicializa os vizinhos na lista do servidor UDP
    udp_server.setUDPNeighbors(neighbors);

    // Carrega os chunks locais do peer
    file_manager.loadLocalChunks();

    // Inicia o servidor TCP em uma thread separada
    std::thread tcp_thread(&TCPServer::run, &tcp_server);

    // Inicia o servidor UDP em uma thread separada
    std::thread udp_thread(&UDPServer::run, &udp_server);

    // Espera para dar tempo de inicializar todos os servidores dos outros peers
    std::this_thread::sleep_for(std::chrono::seconds(Constants::SERVER_STARTUP_DELAY_SECONDS));

    // Threads para busca de cada arquivo
    std::vector<std::thread> threads;

    // Cria uma thread para cada file_name chamando Peer::searchFile e adiciona ao vetor
    for (const auto& file_name : file_names) {
        threads.emplace_back(&Peer::searchFile, this, file_name);
    }

    // Aguarda todas as threads de busca terminarem (join)
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // Espera a finalização das thread do servidor TCP e UDP
    tcp_thread.join();
    udp_thread.join();
}


/**
 * @brief Inicia a busca por chunks de um arquivo na rede.
 */
void Peer::searchFile(const std::string& file_name) {
    // Carrega as informações do arquivo de metadados (nome do arquivo, número total de chunks, e TTL inicial)
    auto [file_name_returned, total_chunks, initial_ttl] = file_manager.loadMetadata(file_name);

    // Verifica se a leitura foi bem-sucedida
    if (total_chunks != -1 && initial_ttl != -1) {
       // Inicializa a estrutura responsável por armazenar as informações de número total de chunks para um arquivo
        file_manager.initializeFileChunks(file_name_returned, total_chunks);

        // Inicializa a estrutura responsável por armazenar informações de localização dos chunks
        file_manager.initializeChunkLocationInfo(file_name_returned);

        // Começa a descoberta dos chunks
        discoverAndRequestChunks(file_name_returned, total_chunks, initial_ttl);
    }
}


/**
 * @brief Inicia o processo de descoberta e solicitação de chunks.
 */
void Peer::discoverAndRequestChunks(const std::string& file_name, int total_chunks, int initial_ttl) {
    // Monta um PeerInfo para o peer original que está enviando a solicitação
    PeerInfo original_sender_info(ip, udp_port);

    // Inicializa como verdadeiro a variável que indica que é permitido processar respostas
    // das mensagens de descoberta de chunks para o arquivo file_name
    udp_server.initializeProcessingActive(file_name);

    // Tenta montar o arquivo com os chunks disponíveis
    bool assembler = file_manager.assembleFile(file_name);

    // Se não conseguir montar o arquivo, envia uma solicitação de descoberta e espera por respostas
    if (!assembler) {
        // Envia a mensagem de descoberta para seus vizinhos
        udp_server.sendChunkDiscoveryMessage(file_name, total_chunks, initial_ttl, original_sender_info);

        // Espera por respostas
        udp_server.waitForResponses(file_name);
    
        // Envia solicitações de chunks aos peers selecionados
        udp_server.sendChunkRequestMessage(file_name);
    } else {
        logMessage(LogType::INFO, "O peer " + std::to_string(id) + " (" + ip + ":" + std::to_string(udp_port) + ") já possuí todos os chunks para " + file_name + ".");
    }
}
