#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "Utils.h"
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * @brief Estrutura que armazena as informações sobre um peer.
 * 
 * A estrutura ChunkLocationInfo guarda os dados essenciais para localizar um peer, como endereço
 * IP, porta UDP de comunicação e a velocidade de transferência em bytes/segundo oferecida.
 */
struct ChunkLocationInfo {
    std::string ip;          ///< Endereço IP do peer.
    int port;                ///< Porta UDP do peer.
    int transfer_speed;      ///< Velocidade de transferência em bytes/segundo do peer.

    /**
     * @brief Construtor da estrutura ChunkLocationInfo.
     * 
     * Inicializa os membros da estrutura com os valores fornecidos ou, caso não sejam fornecidos, 
     * usa valores padrão.
     * 
     * @param ip Endereço IP do peer (padrão: string vazia).
     * @param port Porta UDP do peer (padrão: 0).
     * @param transfer_speed Velocidade de transferência em bytes/segundo do peer (padrão: 0).
     */
    ChunkLocationInfo(const std::string& ip = "", int port = 0, int transfer_speed = 0)
        : ip(ip), port(port), transfer_speed(transfer_speed) {}
};


/**
 * @brief A classe FileManager é responsável pela gestão dos arquivos e chunks disponíveis para um peer em uma rede P2P.
 * 
 * Esta classe oferece funcionalidades para armazenar e gerenciar chunks de arquivos locais, permitindo que o peer 
 * verifique rapidamente quais chunks estão disponíveis e quais precisam ser baixados. Além disso, a FileManager
 * mantém informações detalhadas sobre a localização de chunks em outros peers, facilitando o processo de 
 * download. A classe também possui métodos para selecionar de forma eficiente os peers de onde os chunks 
 * ausentes podem ser adquiridos, garantindo uma transferência de dados otimizada.
 */
class FileManager {
private:
    std::string peer_id;  
    ///< ID do peer.

    std::map<std::string, std::set<int>> local_chunks;
    ///< Mapa que armazena os chunks locais disponíveis para cada arquivo.
    ///< A chave é o nome do arquivo.
    ///< O valor é um conjunto contendo cada ID dos chunks que o peer já possui para aquele arquivo.

    std::map<std::string,std::mutex> local_chunks_mutex;
    ///< Mutexes para proteger o acesso a local_chunks.

    std::unordered_map<std::string, int> file_chunks;
    ///< Mapa que armazena o nome do arquivo que o peer quer buscar como chave
    ///< e o número total de chunks que ele possui como valor.

    std::unordered_map<std::string, std::vector<std::vector<ChunkLocationInfo>>> chunk_location_info;
    ///< Mapa que armazena informações sobre os peers que possuem cada chunk de um arquivo.
    ///< A chave é o nome do arquivo.
    ///< O valor é um vetor onde cada índice representa um chunk do arquivo.
    ///< Cada índice contém um vetor de ChunkLocationInfo, onde cada ChunkLocationInfo descreve um peer.
    ///< que possui o chunk, incluindo seu IP, porta UDP e velocidade de transferência em bytes/segundo.

    std::unordered_map<std::string, std::mutex> chunk_location_info_mutex;
    ///< Mutex para garantir acesso seguro a chunk_location_info.

    std::string directory;  
    ///< Diretório responsável pelo armazenamento dos arquivos do peer, incluindo o local onde novos chunks serão salvos.

public:
    /**
     * @brief Construtor da classe FileManager.
     * 
     * Inicializa um novo FileManager atribuindo um ID único ao peer e configurando o diretório
     * onde os chunks de arquivos serão armazenados localmente. O ID do peer é usado para montar o
     * diretório final.
     * 
     * @param peer_id ID do peer.
     */
    FileManager(const std::string& peer_id);


    /**
     * @brief Carrega os chunks locais disponíveis.
     * 
     * Essa função verifica o diretório do peer e escaneia os arquivos de chunks presentes.
     * A função atualiza a lista de chunks que o peer já possui localmente, facilitando o
     * gerenciamento e verificação dos chunks disponíveis.
     */
    void loadLocalChunks();


    /**
     * @brief Carrega os metadados de um arquivo e retorna as informações.
     * 
     * Lê um arquivo de metadados específico e extrai o nome do arquivo, o número total de chunks
     * e o valor inicial de TTL. Retorna essas informações como uma tupla.
     * 
     * @param file_name Nome do arquivo que se deseja fazer a busca para carregar os metadados.
     * @return Tupla contendo o nome do arquivo, total de chunks e TTL inicial. Retorna {"", -1, -1} se ocorrer um erro ao abrir o arquivo.
     */
    std::tuple<std::string, int, int> loadMetadata(const std::string& file_name);


    /**
     * @brief Inicializa o número de chunks de um arquivo.
     * 
     * Armazena o nome do arquivo e o número total de chunks associados a ele no mapa file_chunks.
     * 
     * @param file_name Nome do arquivo que o peer deseja buscar.
     * @param total_chunks Número total de chunks que compõem o arquivo.
     */
    void initializeFileChunks(const std::string& file_name, int total_chunks);


    /**
     * @brief Inicializa a estrutura para armazenar informações sobre onde encontrar cada chunk.
     * 
     * Esta função prepara o chunk_location_info para armazenar as informações dos peers
     * que possuem cada chunk do arquivo. Ela cria uma entrada no mapa para cada chunk do arquivo,
     * onde as informações dos peers que possuem esse chunk serão armazenadas.
     * 
     * @param file_name O nome do arquivo ao qual o chunk pertence.
     */
    void initializeChunkLocationInfo(const std::string& file_name);


    /**
     * @brief Limpa as informações de localização dos chunks e remove o mutex associado a um arquivo específico.
     * 
     * Remove o file_name do mapa chunk_location_info e apaga os dados de localização de cada chunk,
     * garantindo que a memória associada aos vetores internos seja liberada. Em seguida, remove o mutex 
     * correspondente do mapa chunk_location_info_mutex. É chamado após um assembleFile bem sucedido.
     * 
     * @param file_name Nome do arquivo cujas informações de localização dos chunks devem ser limpas.
     */
    void clearChunkLocationInfo(const std::string& file_name);


    /**
     * @brief Seleciona peers para o download de chunks com base na velocidade de transferência e balanceamento de carga.
     * 
     * Esta função distribui os chunks de um arquivo entre diferentes peers disponíveis, levando em consideração 
     * a velocidade de transferência e a quantidade de chunks já atribuída a cada peer. O objetivo é minimizar o tempo total
     * de download ao selecionar o peer mais rápido disponível para cada chunk, ajustando para evitar sobrecarga em um único peer.
     * 
     * A função prioriza os peers com maior velocidade de transferência e, em caso de empate, atribui o chunk ao peer com menos
     * chunks já alocados, garantindo uma distribuição equilibrada e eficiente. Essa abordagem é independente do tamanho real dos chunks.
     * 
     * @param file_name O nome do arquivo para o qual os chunks serão distribuídos entre os peers.
     * @return Um mapa associando cada peer (identificado por "ip:port") a uma lista de chunks que ele deve solicitar.
     */
    std::unordered_map<std::string, std::vector<int>> selectPeersForChunkDownload(const std::string& file_name);


    /**
     * @brief Armazena informações recebidas sobre a localização dos chunks.
     * 
     * Insere as informações de um peer no mapa chunk_location_info.
     * Essas informações incluem o IP, porta UDP e velocidade de transferência em bytes/segundo do peer que possui tais chunks.
     * A função usa mutexes para garantir que múltiplas threads possam acessar o mapa com segurança.
     * 
     * @param file_name O nome do arquivo associado aos chunks.
     * @param chunk_ids Uma lista de IDs dos chunks que o peer que enviou a resposta possui.
     * @param ip O endereço IP do peer que enviou a resposta.
     * @param port A porta UDP do peer que enviou a resposta.
     * @param transfer_speed A velocidade de transferência em bytes/segundo do peer que enviou a resposta.
     */
    void storeChunkLocationInfo(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed);


    /**
     * @brief Retorna os chunks disponíveis para um arquivo específico.
     * 
     * Essa função retorna uma lista de chunks que já estão disponíveis localmente para um determinado arquivo,
     * permitindo que o peer verifique quais partes do arquivo já foram baixadas ou quais ele pode enviar.
     * 
     * @param file_name Nome do arquivo.
     * @return Vetor contendo os chunks disponíveis localmente.
     */
    std::vector<int> getAvailableChunks(const std::string& file_name);


    /**
     * @brief Retorna o caminho do chunk solicitado.
     * 
     * Retorna o caminho absoluto no sistema de arquivos onde um chunk específico está armazenado.
     * Isso permite que o chunk seja localizado e transferido, se necessário.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return Caminho completo do chunk.
     */
    std::string getChunkPath(const std::string& file_name, int chunk);


    /**
     * @brief Verifica se possui um chunk específico de um arquivo.
     * 
     * Essa função verifica se o peer já possui um chunk específico de um determinado arquivo
     * em seu armazenamento local.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return true se possuir o chunk, false caso contrário.
     */
    bool hasChunk(const std::string& file_name, int chunk);


    /**
     * @brief Salva um chunk recebido no diretório do peer.
     * 
     * Salva os dados recebidos de um chunk no diretório designado do peer. O chunk é gravado
     * no sistema de arquivos para que o peer possa armazená-lo e acessá-lo mais tarde.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @param data Dados do chunk.
     * @param size Tamanho dos dados.
     */
    void saveChunk(const std::string& file_name, int chunk, const char* data, size_t size);


    /**
     * @brief Concatena todos os chunks para formar o arquivo completo.
     * 
     * Combina todos os chunks de um arquivo que foram baixados para formar o arquivo original.
     * 
     * @param file_name Nome do arquivo.
     * @return true se conseguiu criar o novo arquivo com base em todos os chunks ou false, do contrário.
     */
    bool assembleFile(const std::string& file_name);
};

#endif // FILEMANAGER_H
