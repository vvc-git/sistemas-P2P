#ifndef PEER_H
#define PEER_H

#include "ConfigManager.h"
#include "FileManager.h"
#include "TCPServer.h"
#include "UDPServer.h"
#include "Utils.h"
#include <map>
#include <string>
#include <vector>


/**
 * @brief Classe que representa um peer na rede P2P.
 * 
 * Esta classe encapsula todas as funcionalidades relacionadas a um peer 
 * em uma rede peer-to-peer (P2P). Um peer pode descobrir e solicitar chunks 
 * de um arquivo na rede via UDP, transferi-los via TCP, e gerenciá-los
 * localmente através do FileManager.
 */
class Peer {
private:
    const int id;                                                       ///< Identificador único do peer.
    const std::string ip;                                               ///< Endereço IP atribuído ao peer.
    const int udp_port;                                                 ///< Porta UDP usada para descoberta de chunks de um arquivo.
    const int tcp_port;                                                 ///< Porta TCP usada para transferência de chunks de um arquivo.
    const int transfer_speed;                                           ///< Capacidade de transferência de dados do peer em bytes/segundo.
    const std::vector<std::tuple<std::string, int>> neighbors;          ///< Lista de vizinhos diretos do peer, incluindo seus IPs e portas UDP.
    FileManager file_manager;                                           ///< Gerenciador responsável por lidar com os arquivos e chunks do peer.
    TCPServer tcp_server;                                               ///< Servidor TCP usado para transferir chunks de arquivos entre peers.
    UDPServer udp_server;                                               ///< Servidor UDP usado para descoberta de chunks de arquivos na rede P2P.

public:
    /**
     * @brief Construtor da classe Peer. Também inicializa os servidores UDP e TCP e o gerenciador de arquivos.
     * 
     * Inicializa um peer na rede P2P com o ID, IP, portas UDP e TCP, velocidade de 
     * transferência em bytes/segundo e informações sobre seus vizinhos.
     * 
     * @param id Identificador do peer.
     * @param ip Endereço IP do peer.
     * @param udp_port Porta UDP para descoberta de chunks de um arquivo.
     * @param tcp_port Porta TCP para transferência de chunks de um arquivo.
     * @param transfer_speed Capacidade de transferência em bytes/segundo.
     * @param neighbors Informações dos vizinhos do peer (IP, porta UDP).
     */
    Peer(int id, const std::string& ip, int udp_port, 
         int tcp_port, int transfer_speed, 
         const std::vector<std::tuple<std::string, int>> neighbors);


    /**
     * @brief Inicia os servidores TCP e UDP.
     * 
     * Ativa e inicia os servidores TCP e UDP, permitindo que o peer se comunique 
     * na rede P2P para descoberta e transferência de chunks. Dá início a descoberta
     * de chunks de um arquivo.
     * 
     * @param file_names Nomes dos arquivos que se deseja fazer a busca.
     */
    void start(const std::vector<std::string>& file_names);


    /**
     * @brief Inicia a busca por chunks de um arquivo na rede.
     * 
     * Busca chunks de um arquivo específico na rede P2P baseado no arquivo de metadados (.p2p).
     * Utiliza o servidor UDP para descobrir peers que possuem os chunks do arquivo.
     * 
     * @param file_name Nome do arquivo que se deseja fazer a busca.
     */
    void searchFile(const std::string& file_name);


    /**
     * @brief Inicia o processo de descoberta e solicitação de chunks.
     * 
     * Este método envia uma mensagem de descoberta de chunks para encontrar peers 
     * que possuam chunks de um arquivo específico. Em seguida, aguarda 
     * pelas respostas e solicita os chunks disponíveis.
     * 
     * @param file_name Nome do arquivo cujos chunks estão sendo solicitados.
     * @param total_chunks Número total de chunks do arquivo.
     * @param initial_ttl Valor inicial do TTL (time-to-Live) da mensagem de descoberta.
     */
    void discoverAndRequestChunks(const std::string& file_name, int total_chunks, int initial_ttl);
};

#endif // PEER_H
