# Fluxo de desenvolvimento de um projeto STM32CubeIDE com FreeRTOS para **STM32F411CEU5** (BlackPill)

O processo segue o [**mesmo fluxo de um projeto STM32CubeIDE sem FreeRTOS**](https://gitlab.unicamp.br/teaching/bitdoglab/blackpill/hal), com uma diferença importante na etapa de configuração no **STM32CubeMX**: é necessário **ativar o *Middleware* FreeRTOS** e **alterar a base de tempo (*Timebase*)** para um *timer* diferente do **SysTick**, pois o `SysTick` é utilizado pelo FreeRTOS para temporização das tarefas.

1. Criar projeto no [**STM32CubeMX**](https://dca.fee.unicamp.br/pub/docs/ea801/CodeIDE/stm32CubeMX_um1718_user_manual.pdf)
2. Configurar *clock* e periféricos no CubeMX
3. Gerar código-fonte de inicialização
4. Abrir o projeto no [**STM32CubeIDE 2.0.0**](https://dca.fee.unicamp.br/pub/docs/ea801/CodeIDE/um2609-stm32cubeide-user-guide-stmicroelectronics.pdf)
5. Implementar e documentar a lógica do projeto 
  - em arquivos próprios do projeto (recomendado), e/ou
  - apenas em áreas delimitadas por **`USER CODE`** nos arquivos gerados
6. Compilar e gerar o executável
7. Se necessário:
   - voltar ao CubeMX → reconfigurar → regenerar
   - retornar ao IDE → manter lógica do usuário preservada

---
## Configuração no STM32CubeMX (.ioc)

🧩 No **Pinout & Configuration** (diferença do fluxo):
- Em **System Core > SYS**: altere a base de tempo (`Timebase source`) para **NÃO** usar `SysTick`. O **SysTick será utilizado pelo FreeRTOS** para gerenciar a temporização do escalonador e das tarefas (`osDelay`, `vTaskDelay`, etc.). Portanto, é necessário mudar a base de tempo do HAL para outro *timer* disponível. 
  - Debug: Serial Wire
  - Timebase Source: outro *timer* diferente do `SysTick`

> ⭐️ Sempre selecione o timer mais simples e comum disponível (ex.: TIM2) como *Timebase Source* e, após a escolha, confirme em **Timers** e **System Core > NVIC** que ele foi reservado como *system time base* (HAL *timebase*), garantindo que o `SysTick` permaneça livre para o FreeRTOS.

- Em **Middleware and Software Packs**: configure parâmetros operacionais do RTOS, incluindo a gestão de tarefas, semáforos e as definições de hooks do sistema.  
  - Selecione o *middleware*: **FreeRTOS**  
  - Mode: Interface `CMSIS_V2`  
  - Configuration:  
      - Config parameters: *Stack size*, `TICK_RATE_Hz`(frequência com que o *timer* do sistema interrompe a CPU para que o escalonador (scheduler) verifique se há tarefas a serem alternadas)
      - Tasks (*threads*) and Queues: prioridade das *tasks*
      - Timers and Sempahores
      - Mutexes
      - Advanced Settings: `USE_NEWLIB_REENTRANT` e `Use FW pack heap file` 
  
> ⭐️ Recomenda-se começar com uma *task* principal e evoluir a arquitetura aos poucos.  
>  💡 **FreeRTOS + `newlib` / `USE_NEWLIB_REENTRANT`:**  
> A **`newlib`** (biblioteca C padrão usada no GCC) fornece funções como `printf/sprintf`, `malloc/free`, `strtok`, `rand`, etc. Muitas delas mantêm **estado interno global compartilhado**. Em sistemas com **FreeRTOS**, múltiplas *tasks* podem chamar essas rotinas simultaneamente e, se a biblioteca não estiver configurada como **reentrante**, podem ocorrer **corrupção de memória**, resultados incorretos, travamentos e comportamento não determinístico.  
> Para evitar isso, habilite **`USE_NEWLIB_REENTRANT`** (FreeRTOS > Advanced Settings), que faz a `newlib` usar estruturas `struct _reent` **por task**, em vez de globais.  
> >👉 **Habilite** se houver uso concorrente de funções da `libc` em múltiplas *tasks*.  
> >❌ **Não habilite** se não houver concorrência nem uso de alocação dinâmica e houver forte limitação de RAM.

⚙️ Clique em GENERATE CODE.

- Verifique se foram criados/atualizados:
   - `Middlewares/Third_Party/FreeRTOS/`: arquivos do FreeRTOS e CMSIS-RTOS

---

📎 **Nota**

Este projeto foi desenvolvido com fins didáticos, para uso em atividades de laboratório e estudos introdutórios de sistemas embarcados.  
Contou com o suporte de IA para o aprimoramento da redação técnica e organização dos procedimentos.  

