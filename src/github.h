// GitHub 模块：REST API
// 用户信息 + 今日提交（search/commits），有 Token 时额外拉未读通知数
#ifndef _GITHUB_H_
#define _GITHUB_H_

#include <ArduinoJson.h>
#include <time.h>
#include "httpsUtil.h"
#include "settingsExt.h"

struct GithubData
{
  bool ok = false;
  int followers = 0;
  int repos = 0;
  int commitsToday = -1; // -1 = 未知
  int unread = -1;       // -1 = 未配置 Token
  bool unreadCapped = false; // 未读数达到单页上限（显示 N+）
  unsigned long fetchedAt = 0;
};

GithubData github;

// 拉取 GitHub 数据；返回 true 表示有变化（需要刷屏）
bool fetchGithub()
{
  if (strlen(dashSettings.githubUser) == 0)
    return false;

  GithubData g;
  const char *token = strlen(dashSettings.githubToken) > 0 ? dashSettings.githubToken : nullptr;
  String body;

  // 1. 用户信息（粉丝/仓库）
  if (fetchHttps((String("https://api.github.com/users/") + dashSettings.githubUser).c_str(), SECTIGO_E46, token, body, 16384))
  {
    JsonDocument doc;
    if (!deserializeJson(doc, body))
    {
      g.followers = doc["followers"].as<int>();
      g.repos = doc["public_repos"].as<int>();
    }
  }

  // 2. 今日提交数（本地时区的今天）
  time_t now = time(nullptr);
  struct tm *lt = localtime(&now);
  char day[16];
  strftime(day, sizeof(day), "%Y-%m-%d", lt);
  String searchUrl = String("https://api.github.com/search/commits?q=author:") + dashSettings.githubUser +
                     "+committer-date:>=" + day + "&per_page=1";
  if (fetchHttps(searchUrl.c_str(), SECTIGO_E46, token, body, 4096))
  {
    JsonDocument doc;
    if (!deserializeJson(doc, body))
      g.commitsToday = doc["total_count"].as<int>();
  }

  // 3. 未读通知（需要 Token；ESP8266 内存有限，单页条数少）
  if (token)
  {
#ifdef ESP8266
    const int notifPage = 5;
#else
    const int notifPage = 30;
#endif
    if (fetchHttps((String("https://api.github.com/notifications?per_page=") + notifPage).c_str(), SECTIGO_E46, token, body, 16384))
    {
      JsonDocument doc;
      if (!deserializeJson(doc, body))
      {
        g.unread = doc.as<JsonArrayConst>().size();
        g.unreadCapped = (g.unread == notifPage);
      }
    }
  }

  g.ok = true;
  g.fetchedAt = millis();

  bool changed = !github.ok || github.followers != g.followers || github.repos != g.repos ||
                 github.commitsToday != g.commitsToday || github.unread != g.unread;
  github = g;
  Serial.printf("github %s ★%d repos=%d commits=%d unread=%d -> changed=%d\n",
                dashSettings.githubUser, github.followers, github.repos, github.commitsToday, github.unread, changed);
  return changed;
}

#endif
