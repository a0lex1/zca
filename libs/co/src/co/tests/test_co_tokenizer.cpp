#include "co/base/tokenizer.h"
#include "co/base/buffered_tokenizer.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

static void TestTokenizerMatchWith(string subj, string delim = "\n")
{
  syslog(_INFO) << "subj = <" << subj << ">, delim = <" << delim << ">\n";
  
  StringVector buf_toks;
  StringVector stat_toks;
  char* space;
  size_t space_len;
  size_t filled;

  BufferedTokenizer tok_buf(subj.length(), "\n");
  space = tok_buf.GetBufferSpace(space_len);
  DCHECK(space_len == subj.length());
  memcpy(space, subj.c_str(), space_len);
  filled = space_len;
  DCHECK(filled == subj.length());
  tok_buf.OnBufferSpaceFilled(filled);
 
  /* */
  tok_buf.GetAllTokens(buf_toks);
  
  Tokenizer tok_stat("\n", subj.c_str(), subj.length());

  /* */
  tok_stat.GetAllTokens(stat_toks);
  
  syslog(_INFO) << "BufferedTokenizer tokens:\n";
  for (auto& t : buf_toks) {
    syslog(_INFO) << "  " << t << "\n";
  }
  syslog(_INFO) << "(EOL)\n";
  
  syslog(_INFO) << "Tokenizer tokens:\n";
  for (auto& t : stat_toks) {
    syslog(_INFO) << "  " << t << "\n";
  }
  syslog(_INFO) << "(EOL)\n";

  DCHECK(buf_toks == stat_toks);
}

void test_co_tokenizers_match_edgecases(TestInfo& test_info)
{
  // try corrupt
  TestTokenizerMatchWith("");
  TestTokenizerMatchWith("\n");
  TestTokenizerMatchWith("\n\n");
  TestTokenizerMatchWith("\n\n\n\n\n\n\n\n\n\n\n");

  TestTokenizerMatchWith("a\n");
  TestTokenizerMatchWith("a\n\n");
  TestTokenizerMatchWith("a\nb\n");
  TestTokenizerMatchWith("a\nb");
  TestTokenizerMatchWith("a\nb\n\n");

  TestTokenizerMatchWith("\na");
  TestTokenizerMatchWith("\na\n");
  TestTokenizerMatchWith("\n\na");
  TestTokenizerMatchWith("\n\na\n");
  TestTokenizerMatchWith("\n\na\n\n");
  TestTokenizerMatchWith("\n\na\na\n");
}



