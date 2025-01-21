#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Utils.h"
#include <map>
#include <string>
#include <vector>


/**
 * @brief Classe responsável por carregar as informações dos arquivos topologia.txt e config.txt.
 * 
 * Esta classe fornece métodos estáticos para carregar as configurações dos peers e a topologia
 * da rede a partir de arquivos. As configurações incluem informações como IP, porta UDP e
 * velocidade de transferência em bytes/segundo para cada peer, enquanto a topologia fornece
 * informações sobre a sua vizinhança.
 */
class ConfigManager {
public:
    /**
     * @brief Carrega as configurações dos peers a partir do arquivo.
     * 
     * Este método lê um arquivo de configuração e retorna um mapa onde cada chave é o identificador 
     * de um peer e o valor é uma tupla contendo o IP, porta UDP e velocidade de transferência em bytes/segundo desse peer.
     * 
     * @return Mapa com as configurações de cada peer.
     */
    static std::map<int, std::tuple<std::string, int, int>> loadConfig();


    /**
     * @brief Carrega a topologia da rede a partir do arquivo.
     * 
     * Este método lê um arquivo de topologia e constrói um mapa onde cada chave é o identificador 
     * de um peer e o valor é um vetor contendo os identificadores dos seus vizinhos.
     * 
     * @return Mapa com a topologia (vizinhos de cada peer).
     */
    static std::map<int, std::vector<int>> loadTopology();


    /**
     * @brief Expande a topologia com as informações detalhadas da configuração dos peers.
     * 
     * Este método combina a topologia da rede com as informações de configuração de cada peer, 
     * criando um mapa que associa cada peer a uma lista de tuplas, onde cada tupla contém o IP 
     * e a porta de comunicação UDP dos seus vizinhos.
     * 
     * @param topology Mapa da topologia da rede.
     * @param config Mapa de configuração dos peers (IP, porta UDP, velocidade em bytes/segundo).
     * @return Topologia detalhada com as informações dos peers.
     */
    static std::map<int, std::vector<std::tuple<std::string, int>>> expandTopology(
        const std::map<int, std::vector<int>>& topology, 
        const std::map<int, std::tuple<std::string, int, int>>& config
    );
};

#endif // CONFIGMANAGER_H
