#include "co/base/tokenizer.h"
#include "co/base/strings.h"

using namespace std;

namespace co {

Tokenizer::Tokenizer(const char* splitter, const void* user_buf, size_t user_buf_len)
  : TokenizerBase(splitter), user_buf_((const char*)user_buf), user_buf_len_(user_buf_len), cur_pos_(0)
{

}

bool Tokenizer::GetNextToken(string& token)
{
  DCHECK(user_buf_);
  if (cur_pos_ == user_buf_len_) {
    return false;
  }
  const char* cur_pos_ptr = &user_buf_[cur_pos_];
  const char* splitfound = strstr(cur_pos_ptr, GetTokenSplitter());
  if (splitfound == nullptr) {
    // Need more data ended with delimiter like in BufferedTokenizer
    return false;
  }
  size_t slen = splitfound - cur_pos_ptr;
  token = std::string(cur_pos_ptr, slen);
  // Advance
  cur_pos_ += slen + strlen(GetTokenSplitter());
  return true;
}


}



