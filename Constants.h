#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>


namespace Constants {
    // Caminhos dos arquivos
    const std::string BASE_PATH = "./src/";                         ///< Caminho base onde os arquivos do projeto estão armazenados.
    const std::string CONFIG_PATH = BASE_PATH + "config.txt";       ///< Caminho para o arquivo de configuração.
    const std::string TOPOLOGY_PATH = BASE_PATH + "topologia.txt";  ///< Caminho para o arquivo de topologia.

    // Cores para log
    const std::string RESET   = "\033[0m";                          ///< Resetar a cor do texto para branco.
    const std::string RED     = "\033[91m";                         ///< Cor vermelha.
    const std::string GREEN   = "\e[92m";                           ///< Cor verde.
    const std::string YELLOW  = "\033[93m";                         ///< Cor amarela.
    const std::string MAGENTA = "\033[95m";                         ///< Cor magenta.
    const std::string BLUE    = "\033[94m";                         ///< Cor azul.
    const std::string CYAN    = "\033[96m";                         ///< Cor ciano.
    const std::string GRAY    = "\033[37m";                         ///< Cor cinza claro.
    const std::string ORANGE  = "\033[38;5;208m";                   ///< Cor laranja.
    const std::string PINK    = "\033[38;5;213m";                   ///< Cor rosa 
    const std::string PURPLE  = "\033[38;5;177m";                   ///< Cor roxa.
    const std::string GOLD    = "\033[38;5;220m";                   ///< Cor dourado.
    const std::string AQUA    = "\033[38;5;51m";                    ///< Cor ciano claro.

    // Constantes numéricas
    const int DISCOVERY_MESSAGE_INTERVAL_SECONDS = 1;               ///< Tempo de espera em segundos antes de enviar uma mensagem de descoberta para outro vizinho.
    const int SERVER_STARTUP_DELAY_SECONDS       = 5;               ///< Tempo de espera em segundos para inicialização dos servidores.
    const int RESPONSE_TIMEOUT_SECONDS           = 10;              ///< Tempo limite para receber resposta em segundos.
    const int WAIT_TIME_FOR_PORTS_RELEASE_SECONDS= 5;               ///< Tempo de espera em segundos para esperar liberação das portas TCP e UDP.
    const int CONTROL_MESSAGE_MAX_SIZE           = 1024;            ///< Tamanho máximo da mensagem de controle.
    const int TCP_MAX_PENDING_CONNECTIONS        = 10;              ///< Número máximo de conexões pendentes na fila de escuta TCP.
}

#endif // CONSTANTS_H
