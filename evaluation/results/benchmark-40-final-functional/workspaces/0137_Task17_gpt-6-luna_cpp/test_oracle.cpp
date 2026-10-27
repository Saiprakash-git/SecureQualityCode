#include <iostream>
#include <vector>
#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/cpp/Task17_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task17_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task17_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task17_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function



void testToJadenCase() {
    assert(toJadenCase("how can mirrors be real if our eyes aren't real") == "How Can Mirrors Be Real If Our Eyes Aren't Real");
    assert(toJadenCase("the karate kid is a great movie") == "The Karate Kid Is A Great Movie");
    assert(toJadenCase("after earth was released in 2013") == "After Earth Was Released In 2013");
    assert(toJadenCase("i think therefore i am") == "I Think Therefore I Am");
    assert(toJadenCase("to be or not to be") == "To Be Or Not To Be");
    assert(toJadenCase("life is but a dream") == "Life Is But A Dream");
    assert(toJadenCase("it's a beautiful day in the neighborhood") == "It's A Beautiful Day In The Neighborhood");
    assert(toJadenCase("null string should return null") == "Null String Should Return Null");
    assert(toJadenCase("") == ""); // Empty string should remain empty
   // assert(toJadenCase("     ") == ""); // Input with only spaces should return an empty string

    std::cout << "All test cases passed.\n";
}

int main() {
    testToJadenCase();
    return 0;
}