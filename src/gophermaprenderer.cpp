#include "gophermaprenderer.hpp"
#include <cassert>
#include <QTextList>
#include <QTextBlock>
#include <QList>
#include <QStringList>
#include <QTextImageFormat>

#include <QDebug>
#include <QImage>

#include "kristall.hpp"

//Canonical Types
//0   Text File
//1   Gopher submenu or link to another gopher server
//2   CCSO Nameserver
//3   Error code returned by a Gopher server to indicate failure
//4   BinHex-encoded file (primarily for Macintosh computers)
//5   DOS file
//6   uuencoded file
//7   Gopher full-text search
//8   Telnet
//9   Binary file
//+   Mirror or alternate server (for load balancing or in case of primary server downtime)
//g   GIF file
//I   Image file
//T   Telnet 3270
//Non-Canonical Types
//h   HTML file
//i   Informational message
//s   Sound file

std::unique_ptr<QTextDocument> GophermapRenderer::render(const QByteArray &input, const QUrl &root_url, const DocumentStyle &themed_style)
{
    QTextCharFormat standard;
    standard.setFont(themed_style.preformatted_font);
    standard.setForeground(themed_style.preformatted_color);

    QTextCharFormat standard_link;
    standard_link.setFont(themed_style.preformatted_font);
    standard_link.setForeground(QBrush(themed_style.internal_link_color));

    QTextCharFormat external_link;
    external_link.setFont(themed_style.standard_font);
    external_link.setForeground(QBrush(themed_style.external_link_color));

    bool emit_text_only = (global_settings.value("gophermap_display").toString() == "text");

    std::unique_ptr<QTextDocument> result = std::make_unique<QTextDocument>();
    result->setDocumentMargin(themed_style.margin);

    if(not emit_text_only)
    {
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/binary"), QVariant::fromValue(QImage(":/icons/gopher/binary.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/directory"), QVariant::fromValue(QImage(":/icons/gopher/directory.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/dns"), QVariant::fromValue(QImage(":/icons/gopher/dns.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/error"), QVariant::fromValue(QImage(":/icons/gopher/error.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/gif"), QVariant::fromValue(QImage(":/icons/gopher/gif.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/html"), QVariant::fromValue(QImage(":/icons/gopher/html.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/image"), QVariant::fromValue(QImage(":/icons/gopher/image.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/mirror"), QVariant::fromValue(QImage(":/icons/gopher/mirror.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/search"), QVariant::fromValue(QImage(":/icons/gopher/search.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/sound"), QVariant::fromValue(QImage(":/icons/gopher/sound.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/telnet"), QVariant::fromValue(QImage(":/icons/gopher/telnet.svg")));
        result->addResource(QTextDocument::ImageResource, QUrl("gopher/text"), QVariant::fromValue(QImage(":/icons/gopher/text.svg")));
    }

    QTextCursor cursor{result.get()};

    QTextBlockFormat non_list_format = cursor.blockFormat();

    QList<QByteArray> lines = input.split('\n');
    for (auto const &line : lines)
    {
        if (line.length() < 2) // skip lines without
            continue;

        if (line[line.size() - 1] != '\r')
            continue;

        auto items = line.mid(1, line.length() - 2).split('\t');
        if (items.size() < 2) // invalid
            continue;

        QString icon;
        switch (line.at(0))
        {
        case '0': // Text File
            icon = "text";
            break;
        case '1': // Gopher submenu or link to another gopher server
            icon = "directory";
            break;
        case '2': // CCSO Nameserver
            icon = "dns";
            break;
        case '3': // Error code returned by a Gopher server to indicate failure
            icon = "error";
            break;
        case '4': // BinHex-encoded file (primarily for Macintosh computers)
            icon = "binary";
            break;
        case '5': // DOS file
            icon = "binary";
            break;
        case '6': // uuencoded file
            icon = "binary";
            break;
        case '7': // Gopher full-text search
            icon = "search";
            break;
        case '8': // Telnet
            icon = "telnet";
            break;
        case '9': // Binary file
            icon = "binary";
            break;
        case '+': // Mirror or alternate server (for load balancing or in case of primary server downtime)
            icon = "mirror";
            break;
        case 'g': // GIF file
            icon = "gif";
            break;
        case 'I': // Image file
            icon = "image";
            break;
        case 'T': // Telnet 3270
            icon = "telnet";
            break;
        //Non-Canonical Types
        case 'h': // HTML file
            icon = "html";
            break;
        case 'i': // Informational message
            icon = "informational";
            break;
        case 's': // Sound file
            icon = "sound";
            break;
        default: // unknown
            continue;
        }

        QString title = items.at(0);

        // 1Phlog	/phlog	octotherp.org	70	+

        if (line.at(0) == 'i')
        {
            cursor.insertText(title + "\n", standard);
        }
        else
        {
            QString dst_url;
            switch (items.size())
            {
            case 0:
                assert(false);
            case 1:
                assert(false);
            case 2:
                dst_url = root_url.resolved(QUrl(items.at(1))).toString();
                break;
            case 3:
                dst_url = "gopher://" + items.at(2) + "/" + line.mid(0, 1) + items.at(1);
                break;
            default:
                dst_url = "gopher://" + items.at(2) + ":" + items.at(3) + "/" + line.mid(0, 1) + items.at(1);
                break;
            }

            if (not QUrl(dst_url).isValid())
            {
                // invlaid URL generated
                qDebug() << line << dst_url;
            }

            if(emit_text_only)
            {
                cursor.insertText("[" + icon + "] ", standard);
            }
            else
            {
                QTextImageFormat icon_fmt;
                icon_fmt.setName(QString("gopher/%1").arg(icon));
                icon_fmt.setVerticalAlignment(QTextImageFormat::AlignTop);

                cursor.insertImage(icon_fmt);
                cursor.insertText(" ");
            }

            QTextCharFormat fmt = standard_link;
            fmt.setAnchor(true);
            fmt.setAnchorHref(dst_url);
            cursor.insertText(title + "\n", fmt);
        }
    }

    return result;
}
