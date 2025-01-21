#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>


/**
 * @brief Construtor da classe FileManager.
 */
FileManager::FileManager(const std::string& peer_id) : peer_id(peer_id) {}


/**
 * @brief Carrega os chunks locais disponíveis.
 */
void FileManager::loadLocalChunks() {
    // Setando o diretório dos arquivos
    directory = Constants::BASE_PATH + peer_id;
    std::set<std::string> unique_file_names;

    namespace fs = std::filesystem;

    if (!fs::exists(directory)) {
        fs::create_directory(directory);
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        std::string filename = entry.path().filename().string();

        // Formato esperado: <nome>.ch<chunk>
        size_t pos = filename.find(".ch");
        if (pos != std::string::npos) {
            std::string file_name = filename.substr(0, pos);
            int chunk_id = std::stoi(filename.substr(pos + 3));
            local_chunks[file_name].insert(chunk_id);

             unique_file_names.insert(file_name);
        }
    }

    for (const auto& file_name : unique_file_names) {
        local_chunks_mutex.try_emplace(file_name);
    }
}


/**
 * @brief Carrega os metadados de um arquivo e retorna as informações.
 */ 
std::tuple<std::string, int, int> FileManager::loadMetadata(const std::string& file_name) {
    std::string metadata_path = Constants::BASE_PATH + file_name + ".p2p";
    std::ifstream meta_file(metadata_path);
    
    if (!meta_file.is_open()) {
        logMessage(LogType::ERROR, "Erro ao abrir o arquivo de metadados para " + file_name + ". Verifique se o arquivo de metadados " + file_name + ".p2p se encontra em " + Constants::BASE_PATH);
        return {"", -1, -1}; // Retorno padrão em caso de erro
    }

    std::string file_name_returned;
    int total_chunks;
    int initial_ttl;

    // Lê os dados do arquivo de metadados
    std::getline(meta_file, file_name_returned);
    meta_file >> total_chunks;
    meta_file >> initial_ttl;
    meta_file.close();

    return {file_name_returned, total_chunks, initial_ttl}; // Retorna os valores em uma tupla
}


/**
 * @brief Inicializa o número de chunks de um arquivo.
 */
void FileManager::initializeFileChunks(const std::string& file_name, int total_chunks) {
    file_chunks[file_name] = total_chunks;
}


/**
 * @brief Inicializa a estrutura para armazenar informações sobre onde encontrar cada chunk.
 */
void FileManager::initializeChunkLocationInfo(const std::string& file_name) {
    int total_chunks = file_chunks[file_name];

    // Verifica se já existe uma entrada para o file_name
    if (chunk_location_info.find(file_name) == chunk_location_info.end()) {
        chunk_location_info[file_name].resize(total_chunks); // Inicializa com vetores vazios para cada chunk
    }

    // Inicializa os mutexes responsáveis por sincronizar o acesso ao mapa da localização dos chunks de um arquivo
    chunk_location_info_mutex.try_emplace(file_name);
}


/**
 * @brief Limpa as informações de localização dos chunks e remove o mutex associado a um arquivo específico.
 */
void FileManager::clearChunkLocationInfo(const std::string& file_name) {
    // Verifica se o arquivo existe no mapa
    auto it = chunk_location_info.find(file_name);
    if (it != chunk_location_info.end()) {
        // Limpa cada vetor de ChunkLocationInfo para garantir que a memória seja liberada
        for (auto& chunk_info : it->second) {
            chunk_info.clear();
        }
        // Apaga a entrada completa do map
        chunk_location_info.erase(it);
    }

    // Remove o mutex associado ao file_name
    auto mutex_it = chunk_location_info_mutex.find(file_name);
    if (mutex_it != chunk_location_info_mutex.end()) {
        chunk_location_info_mutex.erase(mutex_it);
    }
}


/**
 * @brief Seleciona peers para o download de chunks com base na velocidade de transferência e balanceamento de carga.
 */
std::unordered_map<std::string, std::vector<int>> FileManager::selectPeersForChunkDownload(const std::string& file_name) {
    std::unordered_map<std::string, std::vector<int>> chunks_by_peer_map;
    
     std::vector<std::vector<ChunkLocationInfo>> chunks_with_peer_info;

    {
        std::lock_guard file_lock(chunk_location_info_mutex[file_name]);
        chunks_with_peer_info = chunk_location_info.at(file_name);
    }

    std::size_t total_chunks_in_file = chunks_with_peer_info.size();

    // Itera sobre cada chunk do arquivo
    for (std::size_t chunk_index = 0; chunk_index < total_chunks_in_file; ++chunk_index) {
        const auto& available_peers_for_chunk = chunks_with_peer_info[chunk_index];

        // Verifica se há peers disponíveis para o chunk atual
        if (!available_peers_for_chunk.empty()) {
            // Copia a lista de peers disponíveis e ordena pela velocidade de transferência (decrescente)
            auto sorted_peers_by_speed = available_peers_for_chunk;
            std::sort(sorted_peers_by_speed.begin(), sorted_peers_by_speed.end(), 
                [](const ChunkLocationInfo& a, const ChunkLocationInfo& b) {
                    return a.transfer_speed > b.transfer_speed;
                });

            // Inicializa o peer selecionado como o mais rápido e define a carga mínima como o número de chunks atribuídos a ele
            ChunkLocationInfo selected_peer = sorted_peers_by_speed[0];
            std::string selected_peer_key = selected_peer.ip + ":" + std::to_string(selected_peer.port);
            int min_chunks_assigned = chunks_by_peer_map[selected_peer_key].size();

            // Itera sobre os peers ordenados para encontrar o mais rápido com menos chunks atribuídos
            for (const auto& peer : sorted_peers_by_speed) {
                std::string current_peer_key = peer.ip + ":" + std::to_string(peer.port);
                int chunks_assigned_to_current_peer = chunks_by_peer_map[current_peer_key].size();

                // Se o peer atual tem menos chunks atribuídos, seleciona-o, em caso de empate, mantém a ordem de velocidade
                if (chunks_assigned_to_current_peer < min_chunks_assigned) {
                    selected_peer = peer;
                    selected_peer_key = current_peer_key;
                    min_chunks_assigned = chunks_assigned_to_current_peer;
                }
            }

            // Atribui o chunk ao peer selecionado, adicionando-o ao mapa de chunks para esse peer
            chunks_by_peer_map[selected_peer_key].push_back(static_cast<int>(chunk_index));
        }
    }

    return chunks_by_peer_map;
}


/**
 * @brief Armazena informações recebidas sobre a localização dos chunks.
 */
void FileManager::storeChunkLocationInfo(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed) {
    // Bloqueia o mutex do arquivo uma vez até o final do escopo desse método
    std::lock_guard<std::mutex> file_lock(chunk_location_info_mutex[file_name]);

    // Para cada chunk_id, manipula a lista de peers
    for (const int chunk_id : chunk_ids) {
        // Verifica se o chunk_id está dentro do intervalo
        if (static_cast<size_t>(chunk_id) < chunk_location_info[file_name].size()) {
            // Pega referência direta da lista de chunks e verifica se o peer existe
            auto& chunk_list = chunk_location_info[file_name][chunk_id];
            bool peer_exists = std::any_of(chunk_list.begin(), chunk_list.end(), 
                                           [&](const ChunkLocationInfo& cli) {
                                               return cli.ip == ip && cli.port == port;
                                           });
            // Adiciona o peer caso ele não exista
            if (!peer_exists) {
                chunk_list.emplace_back(ip, port, transfer_speed);
            }
        } else {
            logMessage(LogType::ERROR, "chunk_id " + std::to_string(chunk_id) + " está fora do intervalo para o arquivo: " + file_name);
        }
    }
}


/**
 * @brief Retorna os chunks disponíveis para um arquivo específico.
 */
std::vector<int> FileManager::getAvailableChunks(const std::string& file_name) {
    std::vector<int> available_chunks;

    // Bloqueia o mutex do arquivo uma vez até o final do escopo desse método
    std::lock_guard<std::mutex> file_lock(local_chunks_mutex[file_name]);

    // Verifica se o arquivo está no mapa de chunks locais
    if (local_chunks.find(file_name) != local_chunks.end()) {
        // Copia os chunks disponíveis para o vetor
        available_chunks.assign(local_chunks[file_name].begin(), local_chunks[file_name].end());
    }

    return available_chunks;
}


/**
 * @brief Retorna o caminho do chunk solicitado.
 */
std::string FileManager::getChunkPath(const std::string& file_name, int chunk) {
    return directory + "/" + file_name + ".ch" + std::to_string(chunk);
}


/**
 * @brief Verifica se possui um chunk específico de um arquivo.
 */
bool FileManager::hasChunk(const std::string& file_name, int chunk) {
    // Bloqueia o mutex do arquivo uma vez até o final do escopo desse método
    std::lock_guard<std::mutex> file_lock(local_chunks_mutex[file_name]);

    auto result = local_chunks[file_name].find(chunk) != local_chunks[file_name].end();

    return result;
}


/**
 * @brief Salva um chunk recebido no diretório do peer.
 */
void FileManager::saveChunk(const std::string& file_name, int chunk, const char* data, size_t size) {
    // Bloqueia o mutex do arquivo uma vez até o final do escopo desse método
    std::lock_guard<std::mutex> file_lock(local_chunks_mutex[file_name]);

    std::string path = getChunkPath(file_name, chunk);

    std::ofstream outfile(path, std::ios::binary);
    if (!outfile.is_open()) {
        logMessage(LogType::ERROR, "Não foi possível criar o arquivo para o chunk " + std::to_string(chunk));
        return;
    }

    // Escreve no arquivo                
    outfile.write(data, size);

    // Fecha o arquivo
    outfile.close();

    local_chunks[file_name].insert(chunk); // Armazena o chunk salvo na lista de chunks que possuo
    assembleFile(file_name); // Tenta montar o arquivo
}


/**
 * @brief Concatena todos os chunks para formar o arquivo completo.
 */
bool FileManager::assembleFile(const std::string& file_name) {
    // Sob o bloqueio utilizado em saveChunk
    int total_chunks = file_chunks[file_name];
    bool has_all_chunks = local_chunks[file_name].size() == static_cast<size_t>(total_chunks);

    if (has_all_chunks) {
        int total_chunks = file_chunks[file_name];

        std::string output_path = directory + "/" + file_name;
        std::ofstream output_file(output_path, std::ios::binary);

        for (int i = 0; i < total_chunks; ++i) {
            std::string chunk_path = getChunkPath(file_name, i);
            std::ifstream chunk_file(chunk_path, std::ios::binary | std::ios::in);

            if (!chunk_file.is_open()) {
                logMessage(LogType::ERROR, "Erro ao abrir o chunk " + chunk_path);
                return false;
            }

            output_file << chunk_file.rdbuf();
            chunk_file.close();
        }

        output_file.close();
        displaySuccessMessage(file_name, peer_id);
        clearChunkLocationInfo(file_name);
        return true;
    }
    return false;
}
