import com.sun.source.util.JavacTask;

import javax.tools.Diagnostic;
import javax.tools.DiagnosticCollector;
import javax.tools.JavaCompiler;
import javax.tools.JavaFileObject;
import javax.tools.StandardJavaFileManager;
import javax.tools.ToolProvider;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.List;

/**
 * Parse-only check for Java sources: reports syntax errors without attribution,
 * annotation processing, code generation, or execution.
 * Usage: java -cp DIR JavaParseCheck FILE...
 * Output: one tab-separated line per file: path, ok|error, error count, first message.
 */
public class JavaParseCheck {
    public static void main(String[] args) throws Exception {
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        for (String path : args) {
            DiagnosticCollector<JavaFileObject> diagnostics = new DiagnosticCollector<>();
            try (StandardJavaFileManager fm = compiler.getStandardFileManager(diagnostics, null, StandardCharsets.UTF_8)) {
                Iterable<? extends JavaFileObject> units = fm.getJavaFileObjectsFromFiles(List.of(new File(path)));
                JavacTask task = (JavacTask) compiler.getTask(null, fm, diagnostics, List.of("-proc:none"), null, units);
                task.parse();
            }
            int errors = 0;
            String first = "";
            for (Diagnostic<? extends JavaFileObject> d : diagnostics.getDiagnostics()) {
                if (d.getKind() == Diagnostic.Kind.ERROR) {
                    if (errors == 0) {
                        first = "line " + d.getLineNumber() + ": " + d.getMessage(null).replaceAll("\\s+", " ");
                    }
                    errors++;
                }
            }
            System.out.println(path + "\t" + (errors == 0 ? "ok" : "error") + "\t" + errors + "\t" + first);
        }
    }
}
