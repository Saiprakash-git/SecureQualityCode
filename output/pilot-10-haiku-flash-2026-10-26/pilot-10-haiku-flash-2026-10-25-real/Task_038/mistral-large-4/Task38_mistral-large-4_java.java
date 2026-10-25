import jakarta.servlet.ServletException;
import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;

@WebServlet("/form")
public class FormServlet extends HttpServlet {

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        response.setContentType("text/html;charset=UTF-8");
        try (PrintWriter out = response.getWriter()) {
            out.print(generateFormHtml());
        }
    }

    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        request.setCharacterEncoding("UTF-8");
        String userInput = request.getParameter("userInput");
        response.setContentType("text/html;charset=UTF-8");
        try (PrintWriter out = response.getWriter()) {
            out.print(generateResultHtml(userInput));
        }
    }

    private String generateFormHtml() {
        return "<!DOCTYPE html>\n" +
                "<html>\n" +
                "<head>\n" +
                "    <meta charset='UTF-8'>\n" +
                "    <title>Input Form</title>\n" +
                "</head>\n" +
                "<body>\n" +
                "    <h2>Enter your message</h2>\n" +
                "    <form method='post' action='/form'>\n" +
                "        <input type='text' name='userInput' placeholder='Type something...' required style='padding: 8px; width: 300px;'>\n" +
                "        <button type='submit' style='padding: 8px 16px;'>Submit</button>\n" +
                "    </form>\n" +
                "</body>\n" +
                "</html>";
    }

    private String generateResultHtml(String input) {
        String safeInput = escapeHtml(input);
        return "<!DOCTYPE html>\n" +
                "<html>\n" +
                "<head>\n" +
                "    <meta charset='UTF-8'>\n" +
                "    <title>Result</title>\n" +
                "</head>\n" +
                "<body>\n" +
                "    <h2>You entered:</h2>\n" +
                "    <p style='font-size: 18px; color: #333; padding: 10px; background: #f0f0f0; border-radius: 4px;'>" + safeInput + "</p>\n" +
                "    <a href='/form' style='display: inline-block; margin-top: 20px; padding: 8px 16px; background: #007bff; color: white; text-decoration: none; border-radius: 4px;'>Go back</a>\n" +
                "</body>\n" +
                "</html>";
    }

    private String escapeHtml(String input) {
        if (input == null) {
            return "";
        }
        return input.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;")
                .replace("'", "&#39;");
    }
}