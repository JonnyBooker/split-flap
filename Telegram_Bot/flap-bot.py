#!/usr/bin/env python
import datetime
import logging

from telegram import ForceReply, Update
from telegram.ext import Application, CommandHandler, ContextTypes, MessageHandler, filters

import http.client, urllib.parse

# Enable logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)
logging.getLogger("httpx").setLevel(logging.WARNING)
logger = logging.getLogger(__name__)


async def start(update: Update, _context: ContextTypes.DEFAULT_TYPE) -> None:
    """Send a message when the command /start is issued."""
    user = update.effective_user
    await update.message.reply_html(
        rf"Hi {user.mention_html()}! All your messages will be displayed on the SplitFlap display (for one minute each).",
        reply_markup=ForceReply(selective=True),
    )

async def echo(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:

    """Limit posting frequency to 1/minute."""
    if "last_post" in context.user_data and context.user_data["last_post"] > datetime.datetime.now() - datetime.timedelta(0, 60):
        await update.message.reply_text("Please wait a minute :)")
        return

    message = update.message.text[:10]
    user = update.message.from_user

    logger.info("Message posted by %s: %s", user.username, message)

    params = urllib.parse.urlencode({'message': message})

    try:
        """The ESP accepts urlencoded parameters."""
        headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
        conn = http.client.HTTPConnection("192.168.178.137")
        conn.request("POST", "/remote-message", params, headers)
        response = conn.getresponse()
        conn.close()

        logger.info("Flap Response: %s %s", response.status, response.reason)

        history.append((user.username, message))
    except:
        logger.error("Error connecting to ESP")

    """Update the users last post timestamp."""
    context.user_data["last_post"] = datetime.datetime.now()

async def print_history(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.message.reply_text("\n".join(x[1] for x in history))


def main() -> None:
    """Start the bot."""
    # Create the Application and pass it your bot's token.
    application = Application.builder().token("TOKEN").build()

    application.add_handler(CommandHandler("start", start))

    # on non command i.e message - forward to the ESP
    application.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, echo))
    application.add_handler(CommandHandler("history", print_history))

    application.run_polling(allowed_updates=Update.ALL_TYPES)


if __name__ == "__main__":
    main()
