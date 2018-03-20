/* Copyright [2017] [Comcast, Corp.]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "dmProviderHost.h"
#include "dmProvider.h"

#include "wifi_provider.h"
#include <unistd.h>

dmProviderHost *host = NULL;

class wifiProvider : public dmProvider
{
public:
  wifiProvider()
  {

  }

protected:
  char* getHostName()
  {
    return "WiFiHost";
  }
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
  	char* value = new char[50];
  	if (param.name() == "X_RDKCENTRAL-COM_HostName")
  	{
  	  result.addValue(dmNamedValue(param.name(), getHostName()));
  	}
    else if (param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter Not found");
    }
  	else
  	{
  	  result.addValue(dmNamedValue(param.name(), "netsrvmgr dummy"));
  	}
  	delete[] value;
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if(param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not found");
    }
    else if(param.name() == "nonwritable")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not writable");
    }
  	else
  	  result.addValue(dmNamedValue(param.name(), "Dummy Set"));
  }
};

class wifiEndPointProvider : public dmProvider
{
public:
  wifiEndPointProvider()
  {

  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    char* value = new char[50];
    if (param.name() == "Status")
    {
      result.addValue(dmNamedValue(param.name(), "Online"));
    }
    else if (param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter Not found");
    }
    else
    {
      result.addValue(dmNamedValue(param.name(), "WiFi EndPoint dummy"));
    }
    delete[] value;
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if(param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not found");
    }
    else if(param.name() == "nonwritable")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not writable");
    }
    else
      result.addValue(dmNamedValue(param.name(), "Dummy Set"));
  }
};

class wifiRadioProvider : public dmProvider
{
public:
  wifiRadioProvider()
  {

  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    char* value = new char[50];
    if (param.name() == "Channel")
    {
      result.addValue(dmNamedValue(param.name(), "1"));
    }
    else if (param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter Not found");
    }
    else
    {
      result.addValue(dmNamedValue(param.name(), "WiFi Radio dummy"));
    }
    delete[] value;
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if(param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not found");
    }
    else if(param.name() == "nonwritable")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not writable");
    }
    else
      result.addValue(dmNamedValue(param.name(), "Dummy Set"));
  }
};

class wifiSSIDProvider : public dmProvider
{
public:
  wifiSSIDProvider()
  {

  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    char* value = new char[50];
    if (param.name() == "BSSID")
    {
      result.addValue(dmNamedValue(param.name(), "SSIDName"));
    }
    else if (param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter Not found");
    }
    else
    {
      result.addValue(dmNamedValue(param.name(), "WiFi SSID dummy"));
    }
    delete[] value;
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if (param.name() == " ")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not found");
    }
    else if(param.name() == "nonwritable")
    {
      result.addValue(dmNamedValue(" ", " "), 1, "Parameter not writable");
    }
    else
      result.addValue(dmNamedValue(param.name(), "Dummy Set"));
  }
};

int start_dmProvider()
{
  host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", std::unique_ptr<dmProvider>(new wifiProvider()));
  host->registerProvider("Device.WiFi.EndPoint", std::unique_ptr<dmProvider>(new wifiEndPointProvider()));
  host->registerProvider("Device.WiFi.Radio", std::unique_ptr<dmProvider>(new wifiRadioProvider()));
  host->registerProvider("Device.WiFi.SSID", std::unique_ptr<dmProvider>(new wifiSSIDProvider()));

  return 0;
}

int stop_dmProvider()
{
  if(host)
    host->stop();
  return 0;
}
