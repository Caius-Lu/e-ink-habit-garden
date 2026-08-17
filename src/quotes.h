// 名言模块：LittleFS 文本存储，一行一条，UTF-8；手机网页编辑
#ifndef _QUOTES_H_
#define _QUOTES_H_

#include <LittleFS.h>

const char *quotesFileName = "/quotes.txt";

const char *defaultQuotes =
    "种一棵树最好的时间是十年前，其次是现在。\n"
    "不要等到明天，明天太远了，今天就行动。\n"
    "怕什么真理无穷，进一寸有一寸的欢喜。\n"
    "日拱一卒，功不唐捐。\n"
    "出来混，最重要的是先出来。\n"
    "写代码是为了解决问题，不是为了炫技。\n"
    "早睡早起，从今天开始，从现在开始。\n";

void ensureQuotesFile()
{
  if (LittleFS.exists(quotesFileName))
    return;
  File f = LittleFS.open(quotesFileName, "w");
  if (f)
  {
    f.print(defaultQuotes);
    f.close();
  }
}

String loadQuotesText()
{
  ensureQuotesFile();
  File f = LittleFS.open(quotesFileName, "r");
  if (!f)
    return "";
  String text = f.readString();
  f.close();
  return text;
}

bool saveQuotesText(const String &text)
{
  File f = LittleFS.open(quotesFileName, "w");
  if (!f)
    return false;
  f.print(text);
  f.close();
  return true;
}

int countQuotes()
{
  String text = loadQuotesText();
  int count = 0;
  int from = 0;
  while ((from = text.indexOf('\n', from)) >= 0)
  {
    count++;
    from++;
  }
  if (text.length() > 0 && !text.endsWith("\n"))
    count++;
  return count;
}

// 随机取一条（去掉行尾回车）
String randomQuote()
{
  ensureQuotesFile();
  File f = LittleFS.open(quotesFileName, "r");
  if (!f)
    return "";
  int total = 0;
  while (f.available())
  {
    String line = f.readStringUntil('\n');
    if (line.length() > 0)
      total++;
  }
  if (total == 0)
  {
    f.close();
    return "";
  }
  int pick = random(total);
  f.seek(0);
  int idx = 0;
  while (f.available())
  {
    String line = f.readStringUntil('\n');
    if (line.length() == 0)
      continue;
    if (idx++ == pick)
    {
      f.close();
      line.trim();
      return line;
    }
  }
  f.close();
  return "";
}

#endif
