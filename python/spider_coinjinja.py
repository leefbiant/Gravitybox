# -*- coding: utf-8 -*-
import sys
import time
import requests
import json
import operator as op
from bs4 import BeautifulSoup
import pymysql


def ToString(node):
  if node:
    return json.dumps(node, ensure_ascii=False)
  return ""

def UpdateDb(node):
  db = pymysql.connect("127.0.0.1","root","123456","spider")
  cursor = db.cursor()
  sql = """
  replace into coinjinja(project_id,
                         status,
                         token_for_sale,
                         sold_tokens,
                         name,
                         excerpt,
                         accept_coins,
                         ico_start,
                         ico_end,
                         target_amount_max,
                         initial_price,
                         project_desc,
                         bonus,
                         team,
                         updated_time) 
  value(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s) """ % (
                         node['project_id'],
                         node['status'],
                         node['token_for_sale'],
                         node['sold_tokens'],
                         db.escape(node['name']),
                         db.escape(node['excerpt']),
                         db.escape(node['accept_coins']),
                         db.escape(node['ico_start']),
                         db.escape(node['ico_end']),
                         db.escape(node['target_amount_max']),
                         db.escape(node['initial_price']),
                         db.escape(node['project_desc']),
                         db.escape(node['bonus']),
                         db.escape(node['team']),
                         db.escape(node['updated_time'])
                         )

  #  replace into coinjinja(project_id, status, token_for_sale, sold_tokens, name) 
  #  value(%s, %s, %s, %s, %s)
  #  """ %  (node['project_id'], node['status'], node['token_for_sale'], node['sold_tokens'], db.escape(node['name']))
  print(sql)
  try:
    cursor.execute(sql)
    db.commit()
  except Exception as e:
    print("-------------- UpdateDb:", e)
    db.rollback() 
    db.close()

def GetStringIterm(node, name):
  if name in node and None != node[name]:
    return node[name]
  return "";

def GetIntIterm(node, name):
  if name in node and None != node[name]:
    return node[name]
  return 0;

def GetResp(url):
  print(url)
  time.sleep(1)
  hea = {'User-Agent':'Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36'}
  r = requests.get(url, headers = hea)
  html_doc=str(r.content,'utf-8')
  soup = BeautifulSoup(html_doc, 'lxml')
  tag = soup.find_all('script')
  res_json = "";
  for n in tag:
    for c in n.contents:
      start = c.find("window.__INITIAL_STATE__=")
      end = c.find(";(function()")
      if -1 != start and -1 != end:
        res_json = c[start+len("window.__INITIAL_STATE__="):end]
        break;
  res_dict = json.loads(res_json)
  return res_dict

def GetProjrct():
  url = "https://zh.coinjinja.com"
  page = ""
  index = 1
  while  True:
    try:
      req_url = url + page
      res_dict = GetResp(req_url)
      if res_dict:
        index = index + 1
        page = "/?page=" + str(index);
      else:
        break
      for items in res_dict['ico']['items']:
        node = {}
        node['project_id'] = GetIntIterm(items, 'id');
        node['name'] = items['name']; 
        node['status'] = items['status']; # 0 未开始 1 进行中 2 已成功 3 已结束
        node['excerpt'] = items['excerpt']; #简介
        node['accept_coins'] = ToString(items['accept_coins']); #接受私募币种
        node['ico_start'] = GetStringIterm(items, 'ico_start') #开始时间
        node['ico_end'] = GetStringIterm(items, 'ico_end') #结束时间
        node['token_for_sale'] = GetIntIterm(items, 'token_for_sale'); #发行总量
        node['sold_tokens'] = GetIntIterm(items, 'sold_tokens'); #募集金额
        node['target_amount_max'] = ToString(items['target_amount_max']);#最大募集金额

        # 获取详情
        detail_url = url + "/ico/" + str.lower(node['name'])
        res_detail_dict = GetResp(detail_url)
        if res_detail_dict is None:
          continue
        detail = res_detail_dict['ico']['detail'] #详情
        node['initial_price'] = GetStringIterm(detail, 'initial_price') #价格
        node['project_desc'] = GetStringIterm(detail, 'desc')  #描述 
        node['bonus'] = ToString(GetStringIterm(detail, 'bonus')) #折扣
        node['team'] = GetStringIterm(detail, 'team') #团队
        node['updated_time'] = GetStringIterm(detail, 'updated_at')

        for team in node['team']:
          if op.eq(team['avatar'], "http"):
            continue;
          team['avatar'] = url + team['avatar']
        node['team'] = ToString(node['team'])
        #print(json.dumps(node, indent=4, sort_keys=True, ensure_ascii=False))
        UpdateDb(node);
    except Exception as e:
        print("-------------- Exception:", e)
        continue

if __name__ == "__main__":
    GetProjrct()
