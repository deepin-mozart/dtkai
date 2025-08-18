// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <iostream>

#include <QCoreApplication>

#include <DChatCompletions>

DAI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DChatCompletions chat;

    QList<ChatHistory> historys;
    std::string prompt;

    for (;;) {
        std::cout << "question:\n";
        std::getline(std::cin, prompt);

        if (prompt == "Stop")
            break;

        if (prompt == "Clear") {
            historys.clear();
            continue;
        }

        QString content = QString::fromStdString(prompt);
        QString output = chat.chat(content, historys);
        if (chat.lastError().getErrorCode() != 0) {
            std::cerr << "error:" << chat.lastError().getErrorCode() << std::endl;
            return chat.lastError().getErrorCode();
        }

        std::cout << output.toStdString();

        ChatHistory line;
        line.role = kChatRoleUser;
        line.content = content;
        historys.append(line);

        line.role = kChatRoleAssistant;
        line.content = output;
        historys.append(line);

        std::cout << "\n----------\n";
    }

    return 0;
}
