import os

import anthropic


class ClaudeIntegration:
    def __init__(self, model_name):
        # https://docs.anthropic.com/en/docs/about-claude/models
        # https://console.anthropic.com/settings/plans
        self.model = model_name
        self.client = anthropic.Anthropic(api_key=os.environ["ANTHROPIC_API_KEY"])

    def generate_content(self, prompt_system, prompt_desc):
        message = self.client.messages.create(
            model=self.model,
            max_tokens=4096,
            top_p=0.90,
            temperature=0.9,
            system=prompt_system,
            messages=[
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "text",
                            "text": prompt_desc
                        }
                    ]
                }
            ]
        )
        return message.content
