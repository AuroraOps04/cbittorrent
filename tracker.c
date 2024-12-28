#include "tracker.h"
#include "torrentfile.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned char info_hash[20];
extern char peer_id[20];
extern Announce_list *announce_list_head;
Peer_list *peer_list_head = NULL;
size_t writefunc(void *ptr, size_t size, size_t nmemb, void *userdata) {
  char *buf = (char *)userdata;
  size_t total_size = size * nmemb;
  if (strlen(buf) + total_size > 10000) {
    printf("too large\n");
    return 0;
  }
  strncpy(buf + strlen(buf), ptr, total_size);
  buf[strlen(buf) + total_size] = '\0';
  return total_size;
}

int process_tracker_response(unsigned char *buf, int len) {
  if (buf == NULL) {
    return -1;
  }
  int i, p;
  int flag = 0;
  for (i = 0; i < len; i++) {
    if (buf[i] == '5') {
      if (memcmp(buf + i, "5:peers", 7) == 0) {
        p = i;
        flag = 1;
      }
    }
  }
  if (!flag) {
    // TODO:: get the failed reason
    return -1;
  }
  p += 7;
  // 非紧凑模式
  if (buf[p] == 'l') {
    p++; // skip 'l'
    while (buf[p] != 'e') {
      Peer_list *node = (Peer_list *)malloc(sizeof(Peer_list));
      if (node == NULL) {
        return -1;
      }
      node->next = NULL;
      node->ip = NULL;
      node->port = 0;
      p++; // skip 'd'
      while (buf[p] != 'e') {
        // 2:ip
        // 4:port
        // 7:peer id
        if (buf[p] == '2') {
          p += 4;
          int number = 0;
          while (buf[p] != ':') {
            number = number * 10 + buf[p] - '0';
            p++;
          }
          p++;
          node->ip = (char *)malloc(number + 1);
          memcpy(node->ip, buf + p, number);
          node->ip[number] = '\0';
          p += number;
          p += 6;
          p++; // skip 'i'
          number = 0;
          while (buf[p] != 'e') {
            number = number * 10 + buf[p] - '0';
            p++;
          }
          node->port = number;
          p++; // skip 'e'
        } else {
          p += 9;  // skip '7:peer id'
          p += 3;  // skip '20:'
          p += 20; // skip peer id
        }
      }
      p++; // skip 'e'

      if (peer_list_head == NULL) {
        peer_list_head = node;
      } else {
        Peer_list *q = peer_list_head;
        while (q->next != NULL) {
          q = q->next;
        }
        q->next = node;
      }
    }
  } else {
    // TODO: 紧凑模式
  }
  Peer_list *q = peer_list_head;
  printf("peer list \n");
  while (q != NULL) {
    printf("ip %s port %d\n", q->ip, q->port);
    q = q->next;
  }

  return 0;
}

/**
 * tracker url
 * info_hash
 * peer_id
 * ip optional
 * port
 * uploaded
 * downloaded
 * left
 * event: start completed stopped
 */
int connect_tracker() {
  char *ip = "";
  char *port = "6881";
  int uploaded = 0;
  int downloaded = 0;
  int left = 3000;
  char *event = "start";
  CURL *curl = NULL;
  char url[300];
  unsigned char data[10000] = {0};
  curl = curl_easy_init();
  if (curl == NULL) {
    return -1;
  }
  char *escaped_info_hash = curl_easy_escape(curl, (char *)info_hash, 20);
  char *escaped_peer_id = curl_easy_escape(curl, peer_id, 20);
  sprintf(url,
          "%s?info_hash=%s&peer_id=%s&ip=%s&port=%s&uploaded=%d&downloaded=%d&"
          "left=%d",
          announce_list_head->announce, escaped_info_hash, escaped_peer_id, ip,
          port, uploaded, downloaded, left);
  printf("url %s\n", url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
  int ret = curl_easy_perform(curl);
  if (ret == CURLE_OK) {
    printf("data %s\n", data);
    process_tracker_response(data, strlen(data));
  } else {
    printf("error %s\n", curl_easy_strerror(ret));
  }
  if (curl)
    curl_easy_cleanup(curl);
  return ret;
}

void release_tracker_memory() {
  if (peer_list_head) {
    Peer_list *p = peer_list_head, *tmp;
    while (p != NULL) {
      tmp = p;
      if (tmp->ip) {
        free(tmp->ip);
      }
      p = p->next;
      free(tmp);
    }
    return;
  }
}
