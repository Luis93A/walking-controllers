/**
 * @file WalkingLoggerModule.cpp
 * @authors Giulio Romualdi <giulio.romualdi@iit.it>
 * @copyright 2018 iCub Facility - Istituto Italiano di Tecnologia
 *            Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 * @date 2018
 */

// std
#include <iomanip>

// YARP
#include "yarp/os/LogStream.h"
#include <yarp/os/Time.h>

#include "WalkingLoggerModule.hpp"

double WalkingLoggerModule::getPeriod()
{
    return m_dT;
}

bool WalkingLoggerModule::close()
{
    // close the stream (if it is open)
    if(m_stream.is_open())
        m_stream.close();

    // close the ports
    m_dataPort.close();
    m_rpcPort.close();
}

bool WalkingLoggerModule::respond(const yarp::os::Bottle& command, yarp::os::Bottle& reply)
{
    if (command.get(0).asString() == "quit")
    {
        if(!m_stream.is_open())
        {
            yError() << "[RPC Server] The stream is not open.";
            reply.addInt(0);
            return true;
        }
        m_stream.close();
        reply.addInt(1);

        yInfo() << "[RPC Server] The stream is closed.";
        return true;
    }
    else if (command.get(0).asString() == "record")
    {
        if(m_stream.is_open())
        {
            yError() << "[RPC Server] The stream is already open.";
            reply.addInt(0);
            return false;
        }

        m_numberOfValues = command.size() - 1;

        std::string head{"time "};
        for(int i = 0; i < m_numberOfValues; i++)
            head += command.get(i + 1).asString() + " ";

        yInfo() << "[RPC Server] The following data will be stored: "
                << head;

        // get the current time
        m_time0 = yarp::os::Time::now();

        // set the file name
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        std::stringstream fileName;
        fileName << "Dataset_" << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S")
                 << ".txt";

        m_stream.open(fileName.str().c_str());

        // write the head of the table
        m_stream << head << std::endl;

        reply.addInt(1);
        return true;
    }
    else
    {
        yError() << "[RPC Server] Unknown command.";
        reply.addInt(0);
        return false;
    }
    return true;
}

bool WalkingLoggerModule::configure(yarp::os::ResourceFinder &rf)
{
    // check if the configuration file is empty
    if(rf.isNull())
    {
        yError() << "[configure] Empty configuration for the force torque sensors.";
        return false;
    }

    yarp::os::Value* value;

    // set the module name
    if(!rf.check("name", value))
    {
        yError() << "[configure] Unable to find the module name.";
        close();
        return false;
    }
    if(!value->isString())
    {
        yError() << "[configure] The module name is not a string.";
        close();
        return false;
    }
    std::string name = value->asString();
    setName(name.c_str());

    // set data port name
    if(!rf.check("data_port_name", value))
    {
        yError() << "[configure] Missing field data_port_name.";
        return false;
    }
    if(!value->isString())
    {
        yError() << "[configure] The value is not a string.";
        return false;
    }
    m_dataPort.open("/" + getName() + value->asString());

    // set rpc port name
    if(!rf.check("rpc_port_name", value))
    {
        yError() << "[configure] Missing field rpc_port_name.";
        return false;
    }
    if(!value->isString())
    {
        yError() << "[configure] The value is not a string.";
        return false;
    }
    m_rpcPort.open("/" + getName() + value->asString());
    attach(m_rpcPort);

    // set the RFModule period
    m_dT = rf.check("sampling_time", yarp::os::Value(0.005)).asDouble();

    return true;
}

bool WalkingLoggerModule::updateModule()
{
    yarp::sig::Vector *data = NULL;

    // try to read data from port
    data = m_dataPort.read(false);

    if (data != NULL)
    {
        if(!m_stream.is_open())
        {
            yError() << "[updateModule] No stream is open. I cannot store your data.";
            return false;
        }

        if(data->size() != m_numberOfValues)
        {
            yError() << "[updateModule] The size of the vector is different from "
                     << m_numberOfValues;
            return false;
        }

        // write into the file
        double time = yarp::os::Time::now() - m_time0;
        m_stream << time << " ";
        for(int i = 0; i < m_numberOfValues; i++)
            m_stream << (*data)[i] << " ";

        m_stream << std::endl;
    }
    return true;
}
