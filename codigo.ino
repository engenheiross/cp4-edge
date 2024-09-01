#include <WiFi.h> // Biblioteca para conexão Wi-Fi
#include <PubSubClient.h> // Biblioteca para comunicação MQTT

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* default_PASSWORD = ""; // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "191.232.39.114"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883; // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd"; // Tópico MQTT para receber comandos
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs"; // Tópico MQTT para publicar informações de estado
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l"; // Tópico MQTT para publicar informações de luminosidade
const char* default_ID_MQTT = "fiware_001"; // Identificador único do cliente MQTT
const int default_D4 = 2; // Pino do LED onboard
// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp001";  // Prefixo do tópico MQTT para comandos

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID); // Rede Wi-Fi
char* PASSWORD = const_cast<char*>(default_PASSWORD); // Senha da rede Wi-Fi
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT); // Endereço IP do Broker MQTT
int BROKER_PORT = default_BROKER_PORT;  // Porta do Broker MQTT
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE); // Tópico para escuta
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1); // Tópico para publicação de status
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2); // Tópico para publicação de luminosidade
char* ID_MQTT = const_cast<char*>(default_ID_MQTT); // ID MQTT
int D4 = default_D4;  // Pino do LED onboard

WiFiClient espClient; // Cliente Wi-Fi
PubSubClient MQTT(espClient); // Cliente MQTT utilizando o cliente Wi-Fi
char EstadoSaida = '0'; // Estado atual do LED ('0' para desligado, '1' para ligado)

// Inicializa a comunicação serial
void initSerial() {
    Serial.begin(115200); // Configura a velocidade de comunicação serial
}

// Inicializa a conexão Wi-Fi
void initWiFi() {
    delay(10);  // Atraso para garantir estabilidade
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi(); // Tenta conectar ao Wi-Fi
}

// Inicializa a conexão MQTT
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Configura o servidor MQTT
    MQTT.setCallback(mqtt_callback);  // Define a função de callback para processamento de mensagens MQTT
}

// Função de setup: executa uma vez no início
void setup() {
    InitOutput(); // Inicializa o LED
    initSerial(); // Inicializa a comunicação serial
    initWiFi(); // Inicializa a conexão Wi-Fi
    initMQTT(); // Inicializa a conexão MQTT
    delay(5000);  // Atraso para estabilizar conexões
    MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Publica o estado inicial do LED
}

// Loop principal: executa continuamente
void loop() {
    VerificaConexoesWiFIEMQTT();  // Verifica as conexões Wi-Fi e MQTT
    EnviaEstadoOutputMQTT();  // Envia o estado do LED ao broker MQTT
    handleLuminosity(); // Lê e envia o nível de luminosidade ao broker MQTT
    MQTT.loop();  // Mantém a conexão MQTT ativa
}

// Tenta reconectar ao Wi-Fi se estiver desconectado
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return; // Se já estiver conectado, sai da função
    WiFi.begin(SSID, PASSWORD); // Inicia a conexão Wi-Fi
    while (WiFi.status() != WL_CONNECTED) { // Espera até conectar
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garantir que o LED inicie desligado
    digitalWrite(D4, LOW);
}

// Função de callback para processar mensagens MQTT recebidas
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Formata os tópicos de comparação
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Verifica o comando recebido e ajusta o estado do LED
    if (msg.equals(onTopic)) {  // Comando para ligar o LED
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals(offTopic)) { // Comando para desligar o LED
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// Verifica as conexões Wi-Fi e MQTT e tenta reconectar se necessário
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();  // Tenta reconectar ao broker MQTT
    reconectWiFi(); // Tenta reconectar ao Wi-Fi
}

// Envia o estado do LED ao broker MQTT
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);  // Atraso para não sobrecarregar o broker
}

// Inicializa o estado do LED onboard
void InitOutput() {
    pinMode(D4, OUTPUT);  // Configura o pino do LED como saída
    digitalWrite(D4, HIGH); // Liga o LED inicialmente
    boolean toggle = false;

    // Pisca o LED para indicar inicialização
    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

// Tenta reconectar ao broker MQTT se a conexão for perdida
void reconnectMQTT() {
    while (!MQTT.connected()) { // Continua tentando até conectar
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {  // Tenta conectar
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); // Inscreve-se no tópico de comandos
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);  // Atraso entre tentativas
        }
    }
}

// Lê o valor do sensor de luminosidade e publica no broker MQTT
void handleLuminosity() {
    const int potPin = 34;  // Pino analógico para leitura do sensor de luminosidade
    int sensorValue = analogRead(potPin); // Lê o valor do sensor
    int luminosity = map(sensorValue, 0, 4095, 0, 100); // Mapeia o valor para uma escala de 0 a 100
    String mensagem = String(luminosity); // Converte para string
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str()); // Publica o valor de luminosidade no broker
}