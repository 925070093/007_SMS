#include "httplib.h"
#include "configor/json.hpp"
#include "sha256.hpp"
#include "md5.h"
#include "base64.h"
#include <random>
using namespace configor;
using namespace std;

static string passwd = "oo-^sgaouq";
static string user_id = "925070093";

bool updating = false;
bool check_msm = true;
string sessionId = "";
string tk = "";

string md5(string strPlain)
{
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];

  MD5Init(&mdContext);
  MD5Update(&mdContext, (unsigned char *)const_cast<char *>(strPlain.c_str()), strPlain.size());
  MD5Final(&mdContext);

  string md5;
  char buf[3];
  for (int i = 0; i < 16; i++)
  {
    sprintf(buf, "%02x", mdContext.digest[i]);
    md5.append(buf);
  }
  return md5;
}

void update_sid_tk()
{
  updating = true;
  string s_d = "";
  httplib::Client cli("192.168.0.1", 80);
  string js = "{\n \"cmd\": 232, \"method\": \"GET\", \"sessionId\":\"\", \"language\": \"CN\"\n}\n";
  if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
  {
    if (res->status == 200)
    {
      json j = json::parse(res->body.c_str());
      tk = j["token"].as_string();
      ly::Sha256 sha256;
      string passwd_encode = sha256.getHexMessageDigest(tk + passwd);
      string strMD5_1 = md5(to_string(rand()));
      string strMD5_2 = md5(to_string(rand()));
      string tmp_sessionId = strMD5_1 + strMD5_2;
      for (int i = 0; i < passwd_encode.size(); i++)
      {
        passwd_encode[i] = tolower(passwd_encode[i]);
      }
      js = "{\n \"cmd\":100, \"method\":\"POST\", \"sessionId\":\"" + tmp_sessionId + "\", \"username\": \"superadmin\", \"passwd\":\"" + passwd_encode + "\", \"isAutoUpgrade\": \"0\", \"token\": \"" + tk + "\", \"language\": \"CN\"\n}\n";
      if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
      {
        if (res->status == 200)
        {
          j = json::parse(res->body.c_str());
          if (j["success"].as_bool())
          {
            sessionId = j["sessionId"].as_string();
          }
        }
      }
    }
  }
  updating = false;
}

void check_msm_update()
{
  httplib::Client cli("192.168.0.1", 80);
  string js = "{\n \"cmd\": 12, \"page_num\": 1, \"subcmd\": 0, \"method\": \"GET\", \"language\": \"CN\", \"sessionId\":\"" + sessionId + "\"\n}\n";
  if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
  {
    if (res->status == 200)
    {
      json j = json::parse(res->body.c_str());
      if (j["success"].as_bool())
      {
        string sms_list = j["sms_list"].as_string();
        if (sms_list != "")
        {
          int i = sms_list.find_last_of(",");
          string sms = sms_list.substr(i + 1);
          string sms_decode = base64_decode(sms);
          regex reg("(\\d+) (\\d) (\\d+) (\\d{4}/\\d{2}/\\d{2}) (\\d{2}:\\d{2}:\\d{2}) ([\\s\\S]*)");
          smatch result;
          if (regex_match(sms_decode, result, reg))
          {
            if (result.str(2) == "0")
            {
              js = "{\n \"cmd\": 12, \"method\": \"POST\", \"index\":\"" + result.str(1) + "\", \"token\":\"" + +"\", \"language\": \"CN\", \"sessionId\":\"" + sessionId + "\"\n}\n";
              if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
              {
                if (res->status == 200)
                {
                  string text = "【" + result.str(3) + "】" + result.str(6);
                  httplib::Client cli2("192.168.0.1", 5700);
                  js = "/send_msg?user_id=" + user_id + "&message=" + text;
                  auto res = cli2.Get(js);
                }
              }
            }
          }
        }
      }
      else
      {
        if (!updating)
        {
          update_sid_tk();
        }
      }
    }
  }
}

int main(void)
{
  using namespace httplib;

  Server svr;

  svr.Post("/", [&](const Request &req, Response &resp)
           {
    if (!updating)
    {
      json j = json::parse(req.body.c_str());
      if (j["post_type"] == "message")
      {
        if (j["sub_type"] == "friend" && j["user_id"].as_string() == user_id)
        {
          string message = j["message"].as_string();
          httplib::Client cli("192.168.0.1", 80);
          httplib::Client cli2("192.168.0.1", 5700);
          if (strstr(message.c_str(), "发短信") != NULL)
          {
            regex reg("发短信 ([\\s\\S]+)/([\\s\\S]+)");
            smatch result;
            if (regex_match(message, result, reg))
            {
              string content = base64_encode(result.str(2));
              string js = "{\n \"cmd\": 13, \"method\": \"POST\",\"phoneNo\":\"" + result.str(1) + "\", \"content\":\"" + content + "\", \"token\":\"" + tk + "\", \"language\": \"CN\", \"sessionId\":\"" + sessionId + "\"\n}\n";
              if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
              {
                if (res->status == 200)
                {
                  j = json::parse(res->body.c_str());
                  if (j["success"].as_bool())
                  {
                    auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=发送成功");
                  }
                  else if(!updating)
                  {
                      auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=sessionId失效, 正在更新, 请稍后重试");
                      update_sid_tk();
                  }
                }
              }
            }
            else
            {
              auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=格式：发短信 [号码]/[内容]");
            }
          }

          if (strstr(message.c_str(), "短信通知") != NULL)
          {
            regex reg("短信通知 ([\\s\\S]+)");
            smatch result;
            if (regex_match(message, result, reg))
            {
              if (result.str(1) == "开")
              {
                check_msm = true;
                auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=短信通知已开启, 注意开启后无法登录后台");
              }
              else if (result.str(1) == "关")
              {
                check_msm = false;
                auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=短信通知已关闭");
              }
              else
              {
                auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=格式：短信通知 [开/关]");
              }
            }
            else
            {
              if (check_msm)
              {
                auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=当前状态-【开启】");
              }
              else
              {
                auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=当前状态-【关闭】");
              }
            }
          }

          if (strstr(message.c_str(), "网络状态") != NULL)
          {
            string js = "{\n \"cmd\": 205, \"method\": \"GET\", \"language\": \"CN\", \"sessionId\":\"" + sessionId + "\"\n}\n";
            if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
            {
              if (res->status == 200)
              {
                j = json::parse(res->body.c_str());
                if (j["success"].as_bool())
                {
                  string text = "";
                  text += "网络: " + j["network_type_str"].as_string() + "\n";
                  text += "运营商: " + j["network_operator"].as_string() + "\n";

                  if(j["currentband"].as_string()!=""){
                    text += "频段: B" + j["currentband"].as_string() + "\n";
                  }
                  if (j["currentband_5G"].as_string() != "")
                  {
                    text += "5G频段: N" + j["currentband_5G"].as_string() + "\n";
                  }

                  if (j["RSRP"].as_string() != "")
                  {
                    text += "信号强度: " + j["RSRP"].as_string() + "dBm\n";
                  }
                  if (j["RSRP_5G"].as_string() != "")
                  {
                    text += "5G信号强度: " + j["RSRP_5G"].as_string() + "dBm\n";
                  }

                  if (j["SINR"].as_string() != "")
                  {
                    text += "信号干扰: " + j["SINR"].as_string() + "dB\n";
                  }
                  if (j["SINR_5G"].as_string() != "")
                  {
                    text += "5G信号干扰: " + j["SINR_5G"].as_string() + "dB\n";
                  }

                  if (j["FREQ"].as_string() != "")
                  {
                    text += "频点: " + j["FREQ"].as_string() + "\n";
                  }
                  if (j["FREQ_5G"].as_string() != "")
                  {
                    text += "5G频点: " + j["FREQ_5G"].as_string() + "\n";
                  }

                  if (j["PCI"].as_string() != "")
                  {
                    text += "小区ID: " + j["PCI"].as_string() + "\n";
                  }
                  if (j["PCI_5G"].as_string() != "")
                  {
                    text += "5G小区ID: " + j["PCI_5G"].as_string() + "\n";
                  }
                  text = text.substr(0,text.length()-1);
                  auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message="+text);
                }
                else if (!updating)
                {
                  auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=sessionId失效, 正在更新, 请稍后重试");
                  update_sid_tk();
                }
              }
            }
          }


          if (strstr(message.c_str(), "设备状态") != NULL)
          {
            string js = "{\n \"cmd\": 207, \"method\": \"GET\", \"language\": \"CN\", \"sessionId\":\"" + sessionId + "\"\n}\n";
            if (auto res = cli.Post("/cgi-bin/http.cgi", js, "application/json"))
            {
              if (res->status == 200)
              {
                j = json::parse(res->body.c_str());
                if (j["success"].as_bool())
                {
                  string text = "";
                  text += "设备温度: " + j["device_temperature"].as_string() + "°C\n";
                  text += "CPU占用: " + j["cpu_usage"].as_string() + "％\n";
                  regex reg("(\\d+) kB, (\\d+) kB, (\\d+) kB, (\\d+) kB");
                  smatch result;
                  string memory = j["memory"].as_string();
                  if (regex_match(memory, result, reg))
                  {
                    int memory_total = stoi(result.str(1));
                    int memory_used = stoi(result.str(1)) - stoi(result.str(4));
                    int memory_percent = (int)((float)memory_used / (float)memory_total * 100.0);
                    text += "运行内存: [" + to_string(memory_percent) + "％] " + to_string(memory_used / 1024) + "MB/" + to_string(memory_total / 1024) + "MB\n";
                  }
                  int flash_total = stoi(j["flash_total"].as_string());
                  int flash_used = stoi(j["flash_total"].as_string()) - stoi(j["flash_availeble"].as_string());
                  int flash_percent = (int)((float)flash_used / (float)flash_total * 100.0);
                  text += "储存空间: [" + to_string(flash_percent) + "％] " + to_string(flash_used / 1024) + "MB/" + to_string(flash_total / 1024) + "MB\n";
                  text += "设备型号: " + j["board_type"].as_string() + "\n";
                  text += "固件版本: " + j["real_fwversion"].as_string() + "\n";
                  text += "配置文件版本: " + j["config_version"].as_string() + "\n";
                  text += "设备串号: " + j["module_imei"].as_string() + "\n";
                  text += "系统版本: " + j["system_version"].as_string();
                  auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=" + text);
                }
                else if (!updating)
                {
                  auto res = cli2.Get("/send_msg?user_id=" + user_id + "&message=sessionId失效, 正在更新, 请稍后重试");
                  update_sid_tk();
                }
              }
            }
          }

          if (strstr(message.c_str(), "关闭app") != NULL)
          {
            svr.stop();
            }
        }
      }
      else if (check_msm)
      {
        check_msm_update();
      }
    } });

  svr.Get("/stop", [&](const Request &req, Response &res)
          { svr.stop(); });

  svr.listen("0.0.0.0", 8000);
}