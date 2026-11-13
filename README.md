# Choir Crowd (estilo Asake)

Instrumento virtual de coro "gang vocal" sintetizado: vários cantores com vogal morfável
(Oo → Oh → Ah → Eh → Ee), desafinação e timing humanos, vibrato, respiração, camada de
multidão (murmúrio), reverb e width.

| Sistema | Formatos | Instalador |
|---|---|---|
| macOS 11+ (Apple Silicon **e** Intel — binário Universal) | AU, VST3, Standalone | `ChoirCrowd-<versão>-macOS.pkg` |
| Windows 10/11 (64-bit) | VST3, Standalone | `ChoirCrowd-<versão>-Windows-Setup.exe` |

### Compatibilidade com DAWs

| DAW | macOS | Windows | Formato usado |
|---|---|---|---|
| Logic Pro / GarageBand / MainStage | ✅ | — | AU |
| Ableton Live | ✅ | ✅ | VST3 (Mac também AU) |
| FL Studio | ✅ | ✅ | VST3 |
| Cubase / Nuendo | ✅ | ✅ | VST3 |
| Studio One | ✅ | ✅ | VST3 / AU |
| Reaper | ✅ | ✅ | VST3 / AU |
| Bitwig | ✅ | ✅ | VST3 |
| Reason 12+ | ✅ | ✅ | VST3 |
| Pro Tools | ❌ | ❌ | precisa de **AAX** (ver "Próximos passos") |

> O formato **VST2** já não pode ser distribuído (a Steinberg deixou de licenciar o SDK).
> Todas as DAWs atuais suportam VST3, por isso VST3 + AU cobre praticamente tudo, exceto Pro Tools.

---

## 1. Obter os instaladores (sem instalar nada no teu computador)

O GitHub compila tudo por ti em servidores Mac e Windows.

1. Faz push deste projeto para o GitHub (já está em `kidymusiic-crypto/Mungoz-Choir`).
2. No GitHub abre o separador **Actions** → último workflow **"Build plugin"**.
3. Quando os 3 jobs (`test-linux`, `macos`, `windows`) estiverem verdes, desce até **Artifacts**:
   - `ChoirCrowd-macOS` → contém o **.pkg** (instalador) e um zip "manual".
   - `ChoirCrowd-Windows` → contém o **Setup .exe** e um zip "manual".

### Publicar uma versão oficial (Release com download direto)

```bash
git tag v1.0.0
```

```bash
git push origin v1.0.0
```

O workflow cria automaticamente uma página em **Releases** com o `.pkg` e o `.exe` prontos
para partilhar com qualquer pessoa. Para uma nova versão muda `VERSION` no
`CMakeLists.txt` (`project(ChoirCrowd VERSION 1.0.1)`) e cria a tag `v1.0.1`.

---

## 2. Instalar

### macOS (MacBook Pro)
1. Duplo clique em `ChoirCrowd-<versão>-macOS.pkg`.
2. Se aparecer *"não foi possível verificar o programador"* (acontece enquanto o
   instalador não estiver assinado — ver secção 5): **clique direito → Abrir → Abrir**,
   ou *Definições do Sistema → Privacidade e Segurança → Abrir mesmo assim*.
3. Em **Personalizar** podes escolher AU, VST3 e/ou Standalone. Instala em:
   - AU → `/Library/Audio/Plug-Ins/Components/Choir Crowd.component`
   - VST3 → `/Library/Audio/Plug-Ins/VST3/Choir Crowd.vst3`
   - App → `/Applications/Choir Crowd.app`
4. **Logic Pro** → Definições → Gestor de Plug-ins → *Repor e reanalisar seleção*.
   Aparece em Instrumentos de Software → AudioUnits → **Logogueto → Choir Crowd**.
5. Outras DAWs: faz "Rescan plugins"; aparece como **Logogueto / Choir Crowd** (instrumento).

### Windows
1. Executa `ChoirCrowd-<versão>-Windows-Setup.exe` (pede permissão de administrador).
2. Se o SmartScreen avisar: **Mais informações → Executar mesmo assim**.
3. O VST3 é instalado em `C:\Program Files\Common Files\VST3\Choir Crowd.vst3`
   — a pasta padrão que todas as DAWs analisam automaticamente.
4. Na DAW, faz "Rescan". No **FL Studio**: Options → Manage plugins → *Find more plugins*.
5. Não é preciso instalar nenhum "Visual C++ Redistributable" (o runtime está incluído).

Desinstalar: Windows → *Definições → Aplicações → Choir Crowd → Desinstalar*.
macOS → apaga os 3 ficheiros acima.

---

## 3. Desenvolver (fluxo de trabalho)

```
Source/PluginProcessor.h/.cpp   → o som (DSP) e os parâmetros
Tests/RenderTest.cpp            → teste automático (toca um acorde, verifica som/NaN/estado)
packaging/macos/                → instalador .pkg (pkgbuild/productbuild)
packaging/windows/              → instalador .exe (Inno Setup)
.github/workflows/build.yml     → compila + testa + valida + cria instaladores
```

Ciclo típico:
1. Alteras o código em `Source/`.
2. `git commit` + `git push`.
3. O GitHub Actions compila Mac + Windows, corre o teste de renderização e o
   **pluginval** (o validador usado pela indústria para garantir que o plugin
   não crasha em hosts — testa AU e VST3), e gera os instaladores.
4. Descarregas o instalador dos Artifacts e testas na tua DAW.

### Compilar localmente no Mac (opcional, para iterar mais rápido)

```bash
xcode-select --install
```

```bash
brew install cmake
```

```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

```bash
cmake --build build --config Release
```

O plugin é copiado automaticamente para `~/Library/Audio/Plug-Ins/` (desliga com
`-DCC_COPY_AFTER_BUILD=OFF`). Para gerar o `.pkg` localmente:

```bash
packaging/macos/build_pkg.sh build/ChoirCrowd_artefacts/Release 1.0.0
```

### Compilar localmente no Windows (opcional)
Instala **Visual Studio 2022** (workload "Desenvolvimento para desktop com C++"),
**CMake** e **Inno Setup 6**, depois numa *Developer PowerShell* (como administrador):

```bash
cmake -B build -A x64
```

```bash
cmake --build build --config Release
```

```bash
iscc packaging\windows\ChoirCrowd.iss
```

### Correr o teste automático

```bash
cmake -B build -DCC_BUILD_TESTS=ON -DCC_COPY_AFTER_BUILD=OFF
```

```bash
cmake --build build --target ChoirCrowdTest --config Release
```

Depois executa `build/ChoirCrowdTest_artefacts/Release/ChoirCrowdTest` — deve terminar com `ALL PASSED`.

---

## 4. Som "Asake"
Vogal ~1.6 (entre Oh e Ah), 10–14 Singers, Detune ~18, Humanize 60–90 ms, Crowd 0.4,
Reverb 0.4. Toca acordes abertos em oitavas médias, com notas longas e sustain.
Com acordes de 4+ notas baixa o **Gain** para -6 dB se quiseres menos saturação.

---

## 5. Próximos passos recomendados

1. **Assinatura e notarização Apple** (remove o aviso do macOS). Precisas de uma conta
   Apple Developer (99 USD/ano). Cria os certificados *Developer ID Application* e
   *Developer ID Installer*, exporta-os num `.p12` e adiciona em
   GitHub → Settings → Secrets and variables → Actions:

   | Secret | Valor |
   |---|---|
   | `MAC_CERTS_P12` | o `.p12` em base64 (`base64 -i certs.p12 \| pbcopy`) |
   | `MAC_CERTS_PASSWORD` | password do `.p12` |
   | `MAC_APP_SIGN_ID` | `Developer ID Application: O Teu Nome (TEAMID)` |
   | `MAC_INSTALLER_SIGN_ID` | `Developer ID Installer: O Teu Nome (TEAMID)` |
   | `APPLE_ID` / `APPLE_TEAM_ID` / `APPLE_APP_PASSWORD` | para notarização (app-specific password) |

   O workflow já está preparado: com estes secrets, assina e notariza automaticamente.
2. **Assinatura Windows** (remove o aviso do SmartScreen): certificado de *code signing*
   (ex.: Azure Trusted Signing, ~10 USD/mês) e `signtool` no passo do Inno Setup.
3. **Interface gráfica própria** — neste momento usa o editor genérico do JUCE
   (sliders). Um `PluginEditor.cpp` com knobs e logótipo é o próximo passo visual.
4. **Pro Tools (AAX)** — requer registo gratuito como developer na Avid, o AAX SDK e
   assinatura PACE/iLok. Quando tiveres o SDK, junta `AAX` a `FORMATS` no `CMakeLists.txt`.
5. **CLAP** (opcional, Bitwig/Reaper) — via `clap-juce-extensions`.
6. **Presets** — vários programas (ex.: "Asake Crowd", "Stadium", "Church", "Whisper").
