// ============================================================================
//  CrossCompilePlugin.groovy  --  scientific-calculator
//
//  Applied by every native C++ subproject (calc-core, render-engine,
//  jni-bridge, native-app) via:
//
//      apply plugin: com.calc.build.CrossCompilePlugin
//
//  Bridge between the NEW Gradle Toolchain API and the fault-tolerant
//  provisioner in tools/toolchain-ensure.sh. Concretely it:
//
//    1. Registers (once, at the root project) a `toolchainCheck` Exec task
//       that runs toolchain-ensure.sh --check. Validates host GCC and
//       (re)downloads / caches MinGW-w64 + Zig under .scicalc-cache/.
//       Idempotent and self-heals a wiped toolchain dir.
//
//    2. Makes every CppCompile / Link task in the applied project depend on
//       `toolchainCheck`, so the cache is guaranteed sane before any native
//       code is built.
//
//    3. Configures the project-scope `toolChains {}` extension (NEW API —
//       NO `model { toolChains {} }`) for Gcc and Clang, adding the
//       toolchain cache directories to their search `path()`. This lets
//       Gradle's auto-detection find:
//         - the MinGW cross-compiler for the windows target, and
//         - the Zig-as-clang wrapper scripts for the macOS target.
//
//  NOTE on env injection: CppCompile/Link tasks are NOT Exec tasks and do
//  not expose .environment(). The cross-compiler PATH must reach the
//  compiler subprocess via the toolchain's own `path()` config (done here)
//  and/or by sourcing `tools/toolchain-ensure.sh --env` before running
//  Gradle. Both paths are supported.
// ============================================================================
package com.calc.build

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.language.cpp.tasks.CppCompile
import org.gradle.nativeplatform.tasks.LinkExecutable
import org.gradle.nativeplatform.tasks.LinkSharedLibrary
import org.gradle.nativeplatform.toolchain.Gcc
import org.gradle.nativeplatform.toolchain.Clang
import org.gradle.api.tasks.Exec

class CrossCompilePlugin implements Plugin<Project> {

    @Override
    void apply(Project project) {
        Project root        = project.rootProject
        File ensureScript   = root.file('tools/toolchain-ensure.sh')
        File cacheRoot      = new File(root.projectDir, '.scicalc-cache')
        File tcCache        = new File(cacheRoot, 'toolchains')
        File mingwBin       = new File(tcCache, 'mingw/bin')
        File zigDir         = new File(tcCache, 'zig')
        File wrappersDir    = new File(tcCache, 'wrappers')

        // --- 1. Register the root-level toolchainCheck task (idempotent) ---
        def tccTask = root.tasks.findByName('toolchainCheck')
        if (tccTask == null) {
            tccTask = root.tasks.register('toolchainCheck', Exec).get()
            tccTask.group       = 'Build Setup'
            tccTask.description = 'Validate / (re)provision native toolchains (Linux GCC, MinGW-w64, Zig) with fault tolerance.'
            if (!ensureScript.exists()) {
                tccTask.commandLine 'bash', '-c',
                    "echo '[tc][ERR ] tools/toolchain-ensure.sh missing.' >&2; exit 1"
            } else {
                tccTask.commandLine 'bash', ensureScript.absolutePath, '--check'
            }
            tccTask.environment 'SCI_CALC_TC_CACHE', tcCache.absolutePath
            tccTask.outputs.upToDateWhen { false }
        }

        // --- 2. Every native compile/link depends on the check. ---
        project.tasks.withType(CppCompile).configureEach        { it.dependsOn tccTask }
        project.tasks.withType(LinkExecutable).configureEach    { it.dependsOn tccTask }
        project.tasks.withType(LinkSharedLibrary).configureEach { it.dependsOn tccTask }

        // --- 3. NEW Toolchain API: project-scope `toolChains {}` extension.
        //        Add cache dirs to the toolchain search path so auto-detection
        //        finds MinGW (windows) and the Zig-as-clang wrappers (macOS).
        //        Non-existent dirs are skipped to avoid confusing Gradle on a
        //        fresh checkout before toolchainCheck has run. ---
        project.plugins.withId('native-component') {
            project.toolChains {
                withType(Gcc).configureEach { gcc ->
                    if (mingwBin.exists())  gcc.path mingwBin
                }
                withType(Clang).configureEach { clang ->
                    if (wrappersDir.exists()) clang.path wrappersDir
                    if (zigDir.exists())      clang.path zigDir
                }
            }
        }
    }
}
