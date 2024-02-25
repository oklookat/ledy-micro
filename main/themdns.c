#include "themdns.h"

const char *serviceName = "ledy";
const int port = 80;

void ledyInitMDNS(void)
{
	ESP_ERROR_CHECK(mdns_init());
	ESP_ERROR_CHECK(mdns_hostname_set(serviceName));
	ESP_ERROR_CHECK(mdns_instance_name_set(serviceName));
	char service_type[64];
	sprintf(service_type, "_%s", serviceName);
	ESP_ERROR_CHECK(mdns_service_add(NULL, service_type, "_tcp", port, NULL, 0));
}
