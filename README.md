# Snake Game na BitDogLab

## Descrição
O projeto **Snake Game na BitDogLab** consiste no desenvolvimento de um jogo Snake (Cobrinha) utilizando a placa educacional **BitDogLab**, baseada na **Raspberry Pi Pico**. O jogo será exibido em uma matriz de **LEDs NeoPixel 5x5** e controlado por um **joystick analógico**, proporcionando uma experiência interativa e educativa em **sistemas embarcados**.

## Objetivos
- Proporcionar uma experiência de jogo interativa e educativa em um sistema embarcado de baixo custo.
- Demonstrar a integração de componentes de hardware (**LEDs, joystick, botões**) com software embarcado.
- Servir como ferramenta de aprendizado em programação **C**, manipulação de **GPIOs** e controle de periféricos.

## Requisitos
### Hardware
- **Placa BitDogLab (Raspberry Pi Pico)**
- **Matriz de LEDs NeoPixel 5x5**
- **Joystick analógico**
- **Botão A (conectado ao GPIO5)**

### Software
- **Linguagem C**
- **Biblioteca para controle dos LEDs NeoPixel**
- **Ambiente de desenvolvimento compatível com Raspberry Pi Pico (ex: VS Code com extensões para C/C++)**

## Funcionalidades
- Exibição gráfica da cobrinha (**verde**), frutas (**vermelho**) e paredes (**azul**) na matriz de LEDs.
- Controle da movimentação por **joystick analógico**.
- Crescimento da cobrinha ao **comer frutas**.
- Detecção de colisão e fim de jogo.
- **Pausa e retomada** do jogo através do botão A (**GPIO5**).
- Expansível para integração com **display OLED, buzzers e módulos de comunicação**.

## Como Executar
1. **Configurar o ambiente de desenvolvimento** com as bibliotecas necessárias.
2. **Carregar o código** para a **Raspberry Pi Pico** utilizando o compilador adequado.
3. **Conectar os componentes** conforme os requisitos de hardware.
4. **Executar o jogo** e controlar a cobrinha com o joystick analógico.

## Expansões Futuras
- Adição de **pontuação em um display OLED**.
- Implementação de **efeitos sonoros com buzzers**.
- Comunicação com outros dispositivos via **UART, I2C ou SPI**.

## Licença
Este projeto é de código aberto e pode ser utilizado para fins educacionais e experimentais.


 
