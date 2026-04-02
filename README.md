# Fluxo de desenvolvimento de um projeto STM32CubeIDE com interface CMSIS-RTOS v2 baseada em FreeRTOS para **STM32F411CEU5** (BlackPill)

Em projetos embarcados mais complexos, o uso exclusivo da HAL (*Hardware Abstraction Layer*) não é suficiente para organizar o *firmware* de forma escalável. Embora a HAL permita separar a lógica do programa dos detalhes de *hardware*, ainda permanecem desafios fundamentais, como:

- **execução concorrente** de múltiplas funcionalidades  
- **sincronização** entre eventos assíncronos  
- **manutenção e escalabilidade** do código  

👉 Esses desafios estão relacionados à **estrutura e organização do *firmware***.

Além da organização estrutural, sistemas embarcados frequentemente exigem algo ainda mais importante: o **controle do comportamento no tempo**. Em aplicações sensíveis ao tempo (controle, comunicação, sistemas de tempo real), não basta que o sistema funcione corretamente — ele deve funcionar **no instante correto**.

Isso introduz dois conceitos fundamentais:

- **determinismo** → capacidade de garantir respostas dentro de **limites de tempo conhecidos**  
- **previsibilidade** → capacidade de **antecipar o comportamento temporal**, mesmo sob carga  

## ⚙️ Papel do RTOS

É nesse contexto que se torna necessário o uso de um **RTOS (*Real-Time Operating System*)**, como o FreeRTOS, tipicamente utilizado em microcontroladores STM32 por meio da interface CMSIS-RTOS v2.

O RTOS introduz um **modelo baseado em tarefas concorrentes**, permitindo:  
- organizar a execução do sistema  
- lidar com múltiplas funcionalidades simultâneas  
- preparar o sistema para controle temporal mais rigoroso  

O funcionamento do RTOS é baseado em **escalonamento de tarefas (*task scheduling*)**, isto é **decidir qual tarefa executa e em qual momento**. Essa decisão leva em conta:
- prioridades  
- eventos  
- estado das tarefas  

Para viabilizar o escalonamento, o RTOS depende de uma **base de tempo periódica**, normalmente gerada pelo temporizador **SysTick**, presente nos núcleos ARM Cortex-M.

Essa base permite:  
- manter a contagem de *ticks* do sistema  
- controlar *delays* e temporizações  
- realizar a **troca de contexto entre tarefas**  

👉 É essa base que permite transformar concorrência em **execução temporal organizada**, introduzindo determinismo e previsibilidade.

> ⚠️ **Observação sobre o termo “sistema operacional”**
>
> O termo pode ser considerado limitado nesse contexto, pois:
>
> - não há gerenciamento completo de recursos como em sistemas *desktop*  
> - o RTOS atua principalmente como um **escalonador determinístico de tarefas**
>
> Em essência, ele decide:
>
> **qual tarefa executa, quando e sob quais condições**

## 🏗️ Separação de responsabilidades: HAL + RTOS

| Camada | Responsabilidade |
|--------|-----------------|
| HAL | Abstração de *hardware* (GPIO, UART, ADC, etc.) |
| RTOS | Organização da execução (tarefas, tempo, sincronização) |

Essa separação permite:  
- modularização clara do *firmware*  
- reuso de código  
- redução da complexidade de controle de fluxo  


## 🧩 Estrutura básica de um projeto com RTOS

Um projeto baseado em RTOS é organizado em torno de **tarefas concorrentes** e **mecanismos de sincronização e coordenação** entre elas.

**Tarefas (*tasks* / *threads*)** são unidades independentes de execução. Cada tarefa possui pilha própria e prioridade.  

Exemplo de organização:

- `SensorTask` → leitura periódica de sensores  
- `ControlTask` → processamento e decisão  
- `CommTask` → comunicação  

>👉 Aqui surge formalmente o conceito de **concorrência** que consiste em múltiplas tarefas evoluindo de forma independente  
> 👉 Essa organização também introduz o conceito de **escalabilidade**, que consiste na capacidade de **expandir, modificar ou reutilizar o sistema adicionando novas tarefas ou funcionalidades com baixo acoplamento e impacto mínimo no restante do sistema**

Durante a execução das tarefas, distinguem-se:  
- **execução bloqueante**  
  - a tarefa **aguarda um evento** (sem consumir CPU)  
  - exemplos: `osDelay()`, suspensão da tarefa atual por um intervalo de tempo especificado 

- **execução não bloqueante**  
  - a tarefa continua executando ou cede a CPU de forma controlada  
  - exemplos: uso de *timeouts*, interrupções e *callbacks*  

>👉 O uso adequado desses dois modos de execução é essencial para desempenho e **previsibilidade**  

As tarefas não operam isoladamente. Elas precisam **trocar dados e coordenar ações**.

>👉 Isso introduz o conceito de **sincronização**, que consiste na coordenação entre tarefas e eventos.

Principais mecanismos disponíveis no CMSIS-RTOS:

- **filas (*queues*)** → comunicação de dados entre tarefas (com possível bloqueio/espera)
- **event flags** → sinalização e sincronização baseada em múltiplos eventos
- **semáforos** → sincronização entre tarefas e controle de acesso a recursos

Quando múltiplas tarefas acessam o mesmo recurso compartilhado — por exemplo, duas tarefas escrevendo na mesma variável — surge um problema crítico: o resultado pode se tornar **inconsistente ou imprevisível**, dependendo da ordem de execução.

> 👉 Esse problema envolve dois conceitos fundamentais:
>
> - **condição de corrida (*race condition*)** → ocorre quando o comportamento do sistema depende da ordem e do tempo de execução das tarefas
> - **região crítica** → trecho de código que acessa um recurso compartilhado e que **não deve ser executado simultaneamente por múltiplas tarefas**

Principal mecanismo disponível no CMSIS-RTOS:

- **mutexes** → garantem **exclusão mútua**, permitindo que apenas uma tarefa por vez execute uma região crítica

As tarefas frequentemente precisam **executar ações baseadas no tempo**, como atrasos, *timeouts* ou atividades periódicas.

> 👉 Isso introduz o conceito de **temporização**, que consiste no agendamento e controle da execução de ações ao longo do tempo.

Principais mecanismos disponíveis no CMSIS-RTOS:

- **`osDelay()`** → bloqueia a tarefa atual por um intervalo de tempo definido (baseado no *tick* do sistema)  
- **temporizadores (*timers*)** → executam *callbacks* após um tempo determinado, sem bloquear tarefas. Esses temporizadores de *software* são gerenciados pelo kernel a partir de um *timer* de *hardware* (*tick* do sistema)  
- ***timeouts* em APIs** → permitem limitar o tempo de espera em operações como filas, semáforos e eventos  

>👉 Principais conceitos associados:
>
>- ***one-shot timer*** → executa uma única vez após o tempo configurado  
>- ***periodic timer*** → executa repetidamente em intervalos fixos  
>- ***tick* do sistema** → unidade básica de tempo do RTOS, usada para gerenciar atrasos e temporizadores  
>- ***callback* de *timer*** → função executada automaticamente quando o tempo expira (em contexto do sistema)

## 🔑 Síntese dos conceitos fundamentais

Ao longo da construção de um sistema com RTOS, surgem naturalmente os seguintes conceitos:

- **concorrência** → execução de múltiplas tarefas  
- **sincronização** → coordenação entre tarefas  
- **escalabilidade** → capacidade de evolução modular do sistema com baixo acoplamento  
- **determinismo** → garantia de tempo de resposta  
- **previsibilidade** → comportamento temporal antecipável  
- **execução bloqueante** → espera por eventos sem uso de CPU  
- **execução não bloqueante** → maior controle do fluxo de execução  
- **condição de corrida** → acesso concorrente inseguro  
- **exclusão mútua** → proteção de regiões críticas  
- **região crítica** → trecho de código que acessa recursos compartilhados e deve ser executado de forma exclusiva e atômica  

## 🎯 Deslocamento de concepção: do nível físico para o nível lógico 

Ao desenvolver *firmware* **sem RTOS**, o programador tende a pensar predominantemente no **nível físico**:

- manipulação direta de registradores  
- configuração de *bits* de controle, estado e dados  
- análise de sinais elétricos nos pinos  
- sequenciamento manual do fluxo de execução  

Nesse modelo, o foco está em **como o *hardware* funciona internamente** e em **como produzir os sinais corretos no momento certo**.

Ao adotar **um RTOS**, ocorre um importante **deslocamento de concepção**. O foco deixa de ser “como manipular registradores” e passa a ser **“como coordenar tarefas concorrentes no tempo”**

O desenvolvedor passa a trabalhar predominantemente no **nível lógico do sistema**, concentrando-se em:

- definir **tarefas concorrentes independentes** para cada funcionalidade  
- estruturar o comportamento do sistema em termos de **fluxos concorrentes**  
- organizar a **sincronização entre tarefas**  
- garantir **ordem, prioridade e temporização** das ações  

Nesse novo modelo, o desenvolvimento passa a girar em torno de perguntas como:
- *Qual tarefa deve executar esta funcionalidade?*  
- *Quando essa tarefa deve executar?*  
- *Ela depende de qual evento?*  
- *Como evitar conflitos com outras tarefas?*  

E não mais:  
- *Qual bit preciso setar neste registrador?*  
- *Qual é a sequência exata de sinais no barramento?*  

Para responder a essas questões, o desenvolvedor utiliza os mecanismos do RTOS:

- **filas e *flags*** → comunicação entre tarefas  
- **semáforos e *mutexes*** → sincronização e exclusão mútua  
- ***delays* e *timers*** → controle temporal  

Esses mecanismos permitem:  
- evitar **condições de corrida**  
- garantir **coordenação entre tarefas**  
- assegurar **determinismo e previsibilidade temporal**  

Esse nível mais alto de abstração:
- **simplifica o desenvolvimento**  
- **melhora a organização do *firmware***  
- **favorece escalabilidade e manutenção**  

Porém, em aplicações críticas no tempo: **o domínio do *hardware* continua sendo essencial**. O RTOS não “resolve tudo”. Ele fornece apenas uma base eficiente para:  
- escalonar tarefas  
- sincronizar eventos  
- organizar a execução no tempo  

Na prática:  
- periféricos têm **limitações de tempo e latência**  
- interrupções têm **prioridades e custos de execução**  
- barramentos possuem **restrições físicas e de *throughput***  

Sem esse entendimento, o desenvolvedor pode:  
- escolher mecanismos inadequados  
- introduzir atrasos indesejados  
- comprometer o determinismo do sistema  

>⚠️  O desempenho e a confiabilidade final ainda dependem de **boas decisões de projeto**, que exigem compreensão tanto do **firmware** quanto do **hardware**.
>
>O RTOS **não substitui o conhecimento de *hardware*** — ele apenas fornece uma **infraestrutura de escalonamento e coordenação de tarefas**.

---

## Etapas do processo de desenvolvimento

O processo segue o [**mesmo fluxo de um projeto STM32CubeIDE sem FreeRTOS**](https://gitlab.unicamp.br/teaching/bitdoglab/blackpill/hal), com uma diferença essencial na etapa de configuração no **STM32CubeMX**: é necessário **ativar o *middleware* FreeRTOS**;  e **alterar a base de tempo (*timebase*)** para um *timer* diferente do **SysTick**, pois o `SysTick` é utilizado pelo FreeRTOS para a temporização das tarefas.

1. Criar o projeto no [**STM32CubeMX**](https://dca.fee.unicamp.br/pub/docs/ea801/CodeIDE/stm32CubeMX_um1718_user_manual.pdf).  
2. Configurar o *clock* e os periféricos no CubeMX (HAL).  
3. Estruturar o sistema no *middleware* como um conjunto de (RTOS):
   - tarefas **concorrentes**;  
   - *callbacks* orientados a eventos;  
   - mecanismos de comunicação e sincronização entre tarefas.  
4. Gerar o código de inicialização do sistema, realizando: (1) a configuração dos periféricos, (2) a definição das tarefas que serão executadas e (3) a implementação dos mecanismos que permitem a comunicação e a sincronização entre essas tarefas.  
5. Abrir o projeto no [**STM32CubeIDE 2.0.0**](https://dca.fee.unicamp.br/pub/docs/ea801/CodeIDE/um2609-stm32cubeide-user-guide-stmicroelectronics.pdf).  
6. Implementar a lógica do projeto:
   - em arquivos próprios do usuário;  
   - **apenas nas regiões delimitadas por `USER CODE`** nos arquivos gerados. Tipicamente:
     - lógica das tarefas em `freertos.c`;  
     - *callbacks* orientados a eventos em `main.c`;  
     - mecanismos de coordenação entre tarefas e eventos.  
7. [Documentar](https://gitlab.unicamp.br/teaching/bitdoglab/blackpill/hal) as funções implementadas no projeto.  
8. Compilar e gerar o executável.  
9. Se necessário:
   - retornar ao CubeMX → reconfigurar → regenerar o código;  
   - voltar ao IDE → mantendo a lógica do usuário preservada.  
10. [Versionar o projeto](https://gitlab.unicamp.br/teaching/bitdoglab/blackpill/hal).

---
## 🛠️  Configuração no STM32CubeMX (.ioc)

🧩 No **Pinout & Configuration** (diferença do fluxo):
- Em **System Core > SYS**: altere a base de tempo (`Timebase source`) para **NÃO** usar `SysTick`. O **SysTick será utilizado pelo FreeRTOS** para gerenciar a temporização do escalonador e das tarefas (`osDelay`, `vTaskDelay`, etc.). Portanto, é necessário mudar a base de tempo do HAL para outro *timer* disponível. 
  - Debug: Serial Wire
  - Timebase Source: outro *timer* diferente do `SysTick`

> ⭐️ Sempre selecione o timer mais simples e comum disponível (ex.: TIM2) como *Timebase Source* e, após a escolha, confirme em **Timers** e **System Core > NVIC** que ele foi reservado como *system time base* (HAL *timebase*), garantindo que o `SysTick` permaneça livre para o FreeRTOS.


- Em **Middleware and Software Packs**: configure parâmetros operacionais do RTOS, incluindo a gestão de tarefas, semáforos e as definições de *hooks* do sistema.  
  - Selecione o *middleware*: **FreeRTOS**  
  - Mode: Interface `CMSIS_V2`  
  - Configuration:  
      - Config parameters: *Stack size*, `TICK_RATE_Hz`(frequência com que o *timer* do sistema interrompe a CPU para que o escalonador (*scheduler*) verifique se há tarefas a serem alternadas). Para esta disciplina, usaremos os valores *default* doos parâmetros operacionais do RTOS.
      - Tasks (*threads*) and Queues: inserção das tarefas e filas.
      - *Timers* and *Sempahores*: inserção de *timers* e *semaphores* 
      - Mutexes: inserção de *mutexes*
      - Events: inserção de *flags* de eventos
      - Advanced Settings: `USE_NEWLIB_REENTRANT` e `Use FW pack heap file` 
  
> ⭐️ A configuração neste nível de abstração deve ser realizada **após a definição do modelo de tarefas concorrentes do projeto**, pois é esse modelo que orienta a organização e o comportamento do sistema. Recomenda-se começar com uma *task* principal e evoluir a arquitetura aos poucos.  
>
>  💡 **FreeRTOS + `newlib` / `USE_NEWLIB_REENTRANT`:**  
> A **`newlib`** (biblioteca C padrão usada no GCC) fornece funções como `printf/sprintf`, `malloc/free`, `strtok`, `rand`, etc. Muitas delas mantêm **estado interno global compartilhado**. Em sistemas com **FreeRTOS**, múltiplas *tasks* podem chamar essas rotinas simultaneamente e, se a biblioteca não estiver configurada como **reentrante**, podem ocorrer **corrupção de memória**, resultados incorretos, travamentos e comportamento não determinístico.  
> Para evitar isso, habilite **`USE_NEWLIB_REENTRANT`** (FreeRTOS > Advanced Settings), que faz a `newlib` usar estruturas `struct _reent` **por task**, em vez de globais.  
> >👉 **Habilite** se houver uso concorrente de funções da `libc` em múltiplas *tasks*.  
> >❌ **Não habilite** se não houver concorrência nem uso de alocação dinâmica e houver forte limitação de RAM.

⚙️ Clique em GENERATE CODE.

- Verifique se foram criados/atualizados:
   - `Middlewares/Third_Party/FreeRTOS/`: arquivos do FreeRTOS e CMSIS-RTOS
---
## 🔧 Implementação da lógica do projeto no STM32CubeIDE

A implementação do projeto consiste em desenvolver a lógica das tarefas concorrentes e o tratamento de eventos de interrupção.

De forma geral:

- Implemente a lógica de cada tarefa concorrente no espaço reservado pelo STM32CubeMX, localizado no arquivo `freertos.c`;
- Desenvolva os `callbacks` de tratamento dos eventos de interrupção fornecidos pela HAL no espaço reservado para `USER CODE no arquivo `main.c`.

> Em resumo: as **tarefas** são implementadas em `freertos.c`, enquanto as **interrupções** são tratadas em `main.c`. 

---

📎 **Nota**

Este projeto foi desenvolvido com fins didáticos, para uso em atividades de laboratório e estudos introdutórios de sistemas embarcados.  
Contou com o suporte de IA para o aprimoramento da redação técnica e organização dos procedimentos.  

