#include <stdio.h>

// systems that need initialising
#include "titanium/config/config.hpp"
#include "titanium/filesystem/fs_api.hpp"
#include "titanium/jobsystem/jobs_api.hpp"
#include "titanium/logger/logger.hpp"
#include "titanium/renderer/renderer_api.hpp"

int main()
{
    logger::Info( "wow we're running!" ENDL );
    //filesystem::Initialise( "" );
    //jobsystem::Initialise();

    LOG_CALL( renderer::Initialise() );
}