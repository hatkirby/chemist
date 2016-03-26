#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <twitcurl.h>
#include <verbly.h>

int main(int argc, char** argv)
{
  srand(time(NULL));
  
  YAML::Node config = YAML::LoadFile("config.yml");
    
  twitCurl twitter;
  twitter.getOAuth().setConsumerKey(config["consumer_key"].as<std::string>());
  twitter.getOAuth().setConsumerSecret(config["consumer_secret"].as<std::string>());
  twitter.getOAuth().setOAuthTokenKey(config["access_key"].as<std::string>());
  twitter.getOAuth().setOAuthTokenSecret(config["access_secret"].as<std::string>());
  
  std::map<std::string, std::vector<std::string>> groups;
  std::ifstream datafile("data.txt");
  if (!datafile.is_open())
  {
    std::cout << "Could not find data.txt" << std::endl;
    return 1;
  }
  
  bool newgroup = true;
  std::string line;
  std::string curgroup;
  while (getline(datafile, line))
  {
    if (line.back() == '\r')
    {
      line.pop_back();
    }
    
    if (newgroup)
    {
      curgroup = line;
      newgroup = false;
    } else {
      if (line.empty())
      {
        newgroup = true;
      } else {
        groups[curgroup].push_back(line);
      }
    }
  }
  
  verbly::data database {"data.sqlite3"};
  for (;;)
  {
    std::cout << "Generating tweet" << std::endl;
    std::string action = "{Main}";
    int tknloc;
    while ((tknloc = action.find("{")) != std::string::npos)
    {
      std::string token = action.substr(tknloc+1, action.find("}")-tknloc-1);
      std::string modifier;
      int modloc;
      if ((modloc = token.find(":")) != std::string::npos)
      {
        modifier = token.substr(modloc+1);
        token = token.substr(0, modloc);
      }
      
      std::string canontkn;
      std::transform(std::begin(token), std::end(token), std::back_inserter(canontkn), [] (char ch) {
        return std::toupper(ch);
      });
      
      std::string result;
      if (canontkn == "NOUN")
      {
        result = database.nouns().is_not_proper().random().limit(1).run().front().singular_form();
      } else if (canontkn == "ADJECTIVE")
      {
        result = database.adjectives().random().limit(1).run().front().base_form();
      } else if (canontkn == "VERBING")
      {
        result = database.verbs().random().limit(1).run().front().ing_form();
      } else if (canontkn == "YEAR")
      {
        result = std::to_string(rand() % 100 + 1916);
      } else if (canontkn == "REGION")
      {
        auto hem1 = database.nouns().with_singular_form("eastern hemisphere").limit(1).run().front();
        auto hem2 = database.nouns().with_singular_form("western hemisphere").limit(1).run().front();
        verbly::filter<verbly::noun> region{hem1, hem2};
        region.set_orlogic(true);
        
        result = database.nouns().full_part_holonym_of(region).random().limit(1).run().front().singular_form();
      } else if (canontkn == "FAMOUSNAME")
      {
        auto person = database.nouns().with_singular_form("person").limit(1).run().front();
        auto ptypes = database.nouns().full_hyponym_of({person}).is_class().random().limit(1).run().front();
        result = database.nouns().instance_of({ptypes}).random().limit(1).run().front().singular_form();
      } else {
        auto group = groups[canontkn];
        result = group[rand() % group.size()];
      }
      
      if (modifier == "indefinite")
      {
        if ((result.length() > 1) && (isupper(result[0])) && (isupper(result[1])))
        {
          result = "an " + result;
        } else if ((result[0] == 'a') || (result[0] == 'e') || (result[0] == 'i') || (result[0] == 'o') || (result[0] == 'u'))
        {
          result = "an " + result;
        } else {
          result = "a " + result;
        }
      }
      
      std::string finalresult;
      if (islower(token[0]))
      {
        std::transform(std::begin(result), std::end(result), std::back_inserter(finalresult), [] (char ch) {
          return std::tolower(ch);
        });
      } else if (isupper(token[0]) && !isupper(token[1]))
      {
        auto words = verbly::split<std::list<std::string>>(result, " ");
        for (auto& word : words)
        {
          word[0] = std::toupper(word[0]);
        }
        
        finalresult = verbly::implode(std::begin(words), std::end(words), " ");
      } else {
        finalresult = result;
      }
      
      action.replace(tknloc, action.find("}")-tknloc+1, finalresult);
    }
    
    action.resize(140);

    std::string replyMsg;
    if (twitter.statusUpdate(action))
    {
      twitter.getLastWebResponse(replyMsg);
      std::cout << "Twitter message: " << replyMsg << std::endl;
    } else {
      twitter.getLastCurlError(replyMsg);
      std::cout << "Curl error: " << replyMsg << std::endl;
    }
    
    std::cout << "Waiting" << std::endl;
    sleep(60 * 60);
  }
}
