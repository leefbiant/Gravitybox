# -*- coding:utf-8 -*-
#!/usr/bin/python
import requests
import json
from html.parser import HTMLParser

Access_Key = 'd6b769e9c1b283311518e6ee2cc8c80c'
secret_key = 'dd622fc081b2e040'
url = 'https://m.weibo.cn/api/container/getIndex?containerid=1076036240817051'


class MyHTMLParser(HTMLParser): 
  def __init__(self): 
    HTMLParser.__init__(self) 
    self.a_text = False
    self.a_souece = []
  def handle_starttag(self, tag, attrs): 
    if tag == "a": 
      self.a_text = True

  def handle_endtag(self,tag):
    if tag == 'a':
      self.a_text = False

  def handle_data(self,data):
     if self.a_text:
       self.a_souece.append(data)


def run():
    try :
      params = {
        'access_token': ACCESS_TOKEN,
      }

      res = requests.get(url=URL, params=params)
      if 200 != res.status_code:
        print("error status_code:", (res.status_code))
        return
      data_arry = res.json()['data']['cards']
      for node in data_arry:
        user_info = node['mblog']['user'] 
        print("id:%s screen_name:%s profile_image_url:%s description:%s" % (user_info['id'], user_info['screen_name'], user_info['profile_image_url'], user_info['description']))
        break

      for node in data_arry:
        if 9 != node["card_type"]:
          continue;
        # 文本 && 原文链接
        text = node['mblog']['text']
        scheme = node['scheme']
        print("\r\n---text:%s \r\nscheme:%s" % (text, scheme))

        hp = MyHTMLParser() 
        hp.feed(text) 
        hp.close() 
        print("[html_a_text:%s]" % hp.a_souece)


        # 图片
        if 'pics' in node['mblog']:
          pics = node['mblog']['pics']
          for pic in pics:
            print("url:%s" % (pic['url']))

        # 视频
        if 'page_info' in node['mblog'] and 'media_info' in node['mblog']['page_info']:
          print('media_info:%s' % (node['mblog']['page_info']['media_info']['stream_url']))
    except Exception as e:
      print("-------------- Exception:", e)
      return


if __name__ == "__main__":
  run()
