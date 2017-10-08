
#include <iostream>
#include <cstdint>
#include "serial.h"
#include "util.h"

#if 0
void MainWindow::ReadClock()
{
    int attributeIndex = 2;
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    //Read data from the meter.
    ret = client.Read(&clock, attributeIndex, data);
    if ((ret == 0) && (data.size() > 0))
    {
        Send(data.at(0));
    }
}

void MainWindow::readSocket()
{
    mReply = socket.readAll();

    if (!timer.isActive())
        timer.start(200);

}



void MainWindow::AppendToRequest(const QByteArray &data)
{
    QString txt = ui->requestText->toPlainText();
    txt = txt + data + "\r\n";
    ui->requestText->setPlainText(txt);
}

void MainWindow::AppendToResponse(const QByteArray &data)
{
    QString txt = ui->responseText->toPlainText();
    txt = txt + data + "\r\n";
    ui->responseText->setPlainText(txt);
}



void MainWindow::Send(CGXByteBuffer &request)
{
    QByteArray req((const char *)request.GetData(), request.GetSize());
    Send(req);
}

void MainWindow::connectHdlc()
{
    if (modemState == CONNECTED)
    {
        if (cosemState == HDLC)
        {
            std::vector<CGXByteBuffer> data;

            // FIXME: SNRM does not seems to work, restest after changes made in HDLC framing
             int ret = 0;
             ret = client.SNRMRequest(data);

             if ((ret == 0) && (data.size() > 0))
             {
                 //CGXByteBuffer request = data.at(0);
                 //QByteArray req((const char *)request.GetData(), request.GetSize());
                 QByteArray req = QByteArray::fromHex("7EA0210002002303939A74818012050180060180070400000001080400000007655E7E");
                 Send(req);
             }
        }
/*
        //Get meter's send and receive buffers size.
        if (() != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0 ||
            (ret = m_Parser->ParseUAResponse(reply.GetData())) != 0)
        {
            TRACE("SNRMRequest failed %d.\r\n", ret);
            return ret;
        }
        reply.Clear();
        if ((ret = m_Parser->AARQRequest(data)) != 0 ||
                (ret = ReadDataBlock(data, reply)) != 0 ||
                (ret = m_Parser->ParseAAREResponse(reply.GetData())) != 0)
        {
            if (ret == DLMS_DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
            {
                TRACE1("Use Logical Name referencing is wrong. Change it!\r\n");
                return ret;
            }
            TRACE("AARQRequest failed %d.\r\n", ret);
            return ret;
        }

        */

        /*
        // 7EA0210002002303939A74818012050180060180070400000001080400000007655E7E adresse 17
        //QByteArray req = QByteArray::fromHex("7EA0210002481BF1932188818012050180060180070400000001080400000007655E7E"); // adresse 4621
        QByteArray req = QByteArray::fromHex("7EA0210002002303939A74818012050180060180070400000001080400000007655E7E"); // adresse 17

        // reponse : 7ea023030002002373c0488180140502008006HEX: 02008007040000000708040000000103327e

        //socket.write("/?!\r\n");
        socket.write(req);
        socket.flush();

        */
    }
}


void MainWindow::connectAarq()
{
    if (modemState == CONNECTED)
    {
        std::vector<CGXByteBuffer> data;

        int ret = 0;
        ret = client.AARQRequest(data);

        if ((ret == 0) && (data.size() > 0))
        {
          //  QByteArray req = QByteArray::fromHex("7EA0210002002303939A74818012050180060180070400000001080400000007655E7E");
            Send(data.at(0));
        }

        /*
        QByteArray req = QByteArray::fromHex("6036A1090607608574050801018A0207808B0760857405080201AC0A80084142434445464748BE10040E01000000065F1F04001C1E7D0000");

        socket.write(req);
        socket.flush();
        */
    }
}

#endif

enum ModemState
{
    DISCONNECTED,
    MODEM_OK,
    CONNECTED
};

enum CosemState
{
    HDLC,
    ASSOCIATION_PENDING,
    ASSOCIATED
};

enum Printer
{
    NO_PRINT,
    PRINT_RAW,
    PRINT_HEX
};

class Modem
{
public:
    Modem();

    void Open(const std::string &comport, uint32_t baudrate);
    void Test();
    void Dial(const std::string &phone);
    void Send(const std::string &data, Printer printer);

private:
    ModemState mModemState;
    CosemState mCosemState;
    bool mUseTcpGateway;
    int mSerialHandle;

    static const uint32_t cBufferSize = 40U*1024U;
    char mBuffer[cBufferSize];
};


Modem::Modem()
    : mModemState(DISCONNECTED)
    , mCosemState(HDLC)
    , mUseTcpGateway(false)
    , mSerialHandle(0)
{

}

void Modem::Test()
{
    Send("AT\r\n", PRINT_RAW);
}

void Modem::Open(const std::string &comport, uint32_t baudrate)
{
    mSerialHandle = serial_open(comport.c_str());
    serial_setup(mSerialHandle, baudrate);
}


void Modem::Send(const std::string &data, Printer printer)
{
    if (printer != NO_PRINT)
    {
        if (printer == PRINT_RAW)
        {
            printf("%s", data.c_str());
        }
        else
        {
            print_hex(data.c_str(), data.size());
        }
    }

    if (mUseTcpGateway)
    {
        // TODO
      //  socket.write(data);
      //  socket.flush();
    }
    else
    {
        serial_write(mSerialHandle, data.c_str(), data.size());//sizeof(cnx_hdlc)/2);

        // Immediately read after send, with a timeout guard
        int ret = serial_read(mSerialHandle, &mBuffer[0], cBufferSize);

        if (printer != NO_PRINT)
        {
            if (printer == PRINT_RAW)
            {
                fwrite(&mBuffer[0], ret, 1, stdout);
            }
            else
            {
                print_hex(&mBuffer[0], ret);
            }
        }
    }
}

void Modem::Dial(const std::string &phone)
{
    if (mModemState == MODEM_OK)
    {
        std::string dialRequest = std::string("ATD") + phone + std::string("\r\n");
        Send(dialRequest, PRINT_RAW);
    }
}

void StringToBin(const std::string &in, uint8_t *out)
{
    uint32_t sz = in.size();

    if (!(sz % 2))
    {
        hex2bin(in.c_str(), out, sz);
    }
}

#if 0


void MainWindow::discModem()
{
    QByteArray disc("+++\r\n");
    QByteArray ath0("ATH0\r\n");

    AppendToRequest(disc);
    Send(disc);
    std::this_thread::sleep_for(std::chrono::seconds(1U));

    AppendToRequest(ath0);
    Send(ath0);

    modemState = DISCONNECTED;
    cosemState = HDLC;

    socket.close();
    serial.close();
    ui->connectButton->setText("Disconnect");
    connected = false;
}


void MainWindow::WriteToFile(QString data)
{
    QFile file("gps_output.wor");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
       return;

    QTextStream out(&file);
    out << data << "\n";

}

void MainWindow::startSlave()
{
    if (!connected)
    {
        modemState = DISCONNECTED;
        if (ui->radioTcp->isChecked())
        {
            const int Timeout = 5 * 1000;
            socket.connectToHost(ui->tcpAddress->text(), ui->tcpPort->value());
            if (!socket.waitForConnected(Timeout))
            {
                QString error = socket.errorString();
                processError(error);
                return;
            }
        }
        else
        {
            serial.close();
            serial.setPortName(ui->serialPortComboBox->currentText());

            serial.setDataBits(m_settingsDialog->settings().dataBits);
            serial.setBaudRate(m_settingsDialog->settings().baud);
            serial.setParity(m_settingsDialog->settings().parity);
            serial.setStopBits(m_settingsDialog->settings().stopBits);

            if (!serial.open(QIODevice::ReadWrite)) {
                processError(tr("Can't open %1, error code %2")
                             .arg(serial.portName()).arg(serial.error()));
                return;
            }

        }

        if (!ui->checkUseModem->isChecked())
        {
            modemState = CONNECTED;
        }
        else
        {
            QByteArray checkModem = "AT\r\n";
            AppendToRequest(checkModem);
            Send(checkModem, false);
        }

        connected = true;
        ui->connectButton->setText("Disconnect");
        statusBar()->showMessage(tr("Port open."), 5000);
    }
    else
    {
        if (ui->radioTcp->isChecked())
        {
            discModem();
        }
        else
        {
            serial.close();
        }

        connected = false;
        ui->connectButton->setText("Connect");
        statusBar()->showMessage(tr("Port closed."), 5000);
    }
}


void MainWindow::readRequest()
{
    while (!serial.atEnd()) {
      mReply += serial.read(serial.bytesAvailable());
    }

    if (!timer.isActive())
        timer.start(200);

}

void MainWindow::Decode(const QByteArray &response)
{
    if (modemState == CONNECTED)
    {
        CGXByteBuffer buffer;
        buffer.Set(response.data(), response.size());

        if (cosemState == HDLC)
        {
            if (client.ParseUAResponse(buffer) == 0)
            {
                cosemState = ASSOCIATION_PENDING;
            }
        } else if (cosemState == ASSOCIATION_PENDING)
        {
         //   if (client.ParseAAREResponse(buffer) == 0)
            {
                cosemState = ASSOCIATED;
            }
        } else if (cosemState == ASSOCIATED)
        {
            // Lecture de l'heure:
            // 7ea021030002002352f04fe6e700c401c100090c07e0010906152638ff80000095237e
            CGXReplyData reply;
            client.GetData(buffer, reply);
            CGXDLMSVariant replyData = reply.GetValue();
            if (client.UpdateValue(clock, 2, replyData) == 0)
            {
                std::string time = clock.GetTime().ToString();
                QByteArray timeString = "Meter time: " + QByteArray(time.c_str());
                AppendToResponse(timeString);
            }
        }
    }
    else
    {
        QString modemReply = response.toStdString() .c_str();

        switch(modemState)
        {
        case DISCONNECTED:
            if (modemReply.contains("OK"))
            {
                modemState = MODEM_OK;
            }
            break;
        case MODEM_OK:
            if (modemReply.contains("CONNECT"))
            {
                modemState = CONNECTED;
            }
            break;
        case CONNECTED:
        default:
             break;
        }
    }

    AppendToResponse(response.toHex());
}

void MainWindow::processTimeout()
{
    timer.stop();
    Decode(mReply);
    mReply.clear();
}

#endif

static const uint8_t snrm[] = {0x7E, 0xA0, 0x21, 0x00, 0x02, 0x00, 0x23, 0x03, 0x93, 0x9A, 0x74, 0x81, 0x80, 0x12,
                               0x05, 0x01, 0x80, 0x06, 0x01, 0x80, 0x07, 0x04, 0x00, 0x00, 0x00, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0x07, 0x65, 0x5E, 0x7E };

#define BUF_IN_SIZE sizeof(buf_in)
#define BUF_OUT_SIZE sizeof(buf_out)



int main(int argc, char **argv)
{
    Modem modem;

    if (argc >= 3)
    {
        modem.Open(argv[1], 9600);
        modem.Test();
        modem.Dial(argv[2]);
    }
    else
    {
        printf("Usage: cosem_client /dev/ttyUSB0 0244059867\r\n");
    }

    return 0;

}
