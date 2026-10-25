package edu.bzu.llm;

import io.github.stefanbratanov.jvm.openai.*;
import java.util.Objects;

public class GeminiIntegration {

    public static void main(String[] args) {
        // Define the prompt
        String prompt = "Explain the benefits of using Java for web development.";

        // Send the request and handle the response
        try {
                OpenAI openAI = OpenAI.newBuilder(
                    Objects.requireNonNull(System.getenv("OPENAI_API_KEY"), "OPENAI_API_KEY is required")
                ).build();

            ChatClient chatClient = openAI.chatClient();
            CreateChatCompletionRequest createChatCompletionRequest = CreateChatCompletionRequest.newBuilder()
                    .model(OpenAIModel.GPT_3_5_TURBO)
                    .message(ChatMessage.userMessage(prompt))
                    .build();
            ChatCompletion chatCompletion = chatClient.createChatCompletion(createChatCompletionRequest);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}