#include <AdsDef.h>
#include <AdsDevice.h>
#include <AdsLib.h>

void NotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser);
void notificationByName(std::ostream& out, const AdsDevice& route, std::string variableName, int variableType, std::string iGroup);
std::string removeTrailingSpaces(std::string str);
std::string removeLeadingSpaces(std::string str);
int *extractAMS(char *);
int ConnectToTwinCATAds(UA_Server *); //char*, char*, int, UA_Server *);
