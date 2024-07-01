
#include <iostream>
#include <nlohmann/json.hpp>

#include "../src/web.hpp"

using namespace rfs;
using namespace std;

class AnglesHandler: public HTTPEventHandler {

public:

    virtual void post(HTTPRequest &request) const override
    {
        expected<nlohmann::json, nlohmann::json::parse_error> result_json = request.json();
        if (!result_json) {
            cerr << "Error parsing" << endl;
            reply(request, HTTP_ERROR_500_SERVER_ERROR, result_json.error().what());
            return;
        }
        cout << request.body() << endl;
        ok(request);
    }

};

int main()
{
    WebApplication app(true);
    std::unique_ptr<HTTPEventHandler> site_handler = std::make_unique<HTTPFileHandler>("../test/index.html");
    std::unique_ptr<HTTPEventHandler> angles_handler = std::make_unique<AnglesHandler>();
    app.listen("http://0.0.0.0:8000");
    app.add_handler("/", site_handler);
    app.add_handler("/angles/", angles_handler);

    while (1) {
        app.poll(1000);
    }
}