# Runtime: o que é e como vai ser feito

O jogo é um `.exe` de Windows x64 feito na Unreal Engine 5. O PS4 também é x86-64, então não há instruções para traduzir.
O trabalho é fazer o `.exe` rodar em cima do PS4 (sistema diferente, API gráfica diferente, 4,5 GiB de memória direta).

## Fases
1. **Análise (v00.07, pronta):** o botão **Start** acha o `.exe`, lê a lista de DLLs e funções do Windows que ele importa,
   detecta a API gráfica (D3D12/D3D11/Vulkan) e middlewares (Steam, EOS, Oodle, Bink, FMOD) e mede a pasta do jogo.
   Relatório completo em `/data/Titans/logs/runtime_report.txt`.
2. **Carregador PE em simulação:** mapear as seções, aplicar relocações e ligar cada import a um stub que só registra
   a chamada. Sem executar o jogo.
3. **Execução da entrada:** estado de thread do Windows (TEB/TLS), convenção de chamada Windows x64 → PS4, threads e
   sincronização, memória. Cada API não implementada aparece no log e é implementada na ordem em que o jogo chama.
   A confirmar aqui: se o PS4 permite memória executável criada em tempo de execução. Se não permitir, o código do `.exe`
   precisa entrar como segmento executável do ELF no build (conversão PE → ELF no GitHub Actions; hoje o jogo fica só no PS4).
4. **Sistema:** arquivos (`.pak/.utoc/.ucas`), tempo, entrada (DualShock), áudio.
5. **Gráficos (a fase que decide a viabilidade):** a UE5 usa D3D12 (Shader Model 6, Nanite, Lumen). Precisa de uma camada
   D3D12 → GPU do PS4 e de tradução de shaders. O hardware do PS4 pode não suportar alguns recursos; o plano seria rodar
   com esses recursos desligados.

## O que as opções do Settings fazem
Ficam em `/data/Titans/config/settings.ini`. O runtime vai ler esse arquivo no boot (resolução, render scale, FPS,
sombras, etc.).

## Próximo passo
Rode **Start** no PS4 e me mande `/data/Titans/logs/runtime_report.txt`. Com a lista real de imports e a API gráfica
detectada, dá para dimensionar as fases 2 a 5 com dados.

Se o `.exe` não for achado sozinho, crie `/data/Titans/config/game.ini` com `exe=caminho/relativo/dentro/de/game.exe`.
