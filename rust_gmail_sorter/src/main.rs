extern crate google_gmail1 as gmail1;
extern crate hyper;
extern crate hyper_rustls;

use gmail1::api::{BatchModifyMessagesRequest, MessagePartHeader};
use gmail1::{oauth2, Gmail};
use itertools::Itertools;
use pancurses::Input::Character;
use std::collections::HashMap;
use std::default::Default;
use std::fmt::Debug;
use std::ops::Deref;

fn get_bracketed(s:&str) ->&str{
    s.split_once("[").unwrap_or(("","")).1.split_once("]").unwrap_or(("","")).0
}

#[cfg(test)]
mod tests {
    use crate::get_bracketed;

    #[test]
    fn test_get_bracketed() {
        assert_eq!(get_bracketed("this is [ a very long test [ ] ] "), " a very long test [ ");
    }
}

#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq)]
enum Status {
    Inbox,
    FollowUp,
    ReadThrough,
    Archive,
}

#[derive(Debug)]
struct Email {
    status: Status,
    subject: String,
    from_name: String,
    from_domain: String,
    body: String,
    date: i64,
    id: String,
}

impl ToString for Status {
    fn to_string(&self) -> String {
        match self {
            Status::Inbox => "Inbox".to_owned(),
            Status::FollowUp => "FollowUp".to_owned(),
            Status::ReadThrough => "ReadThrough".to_owned(),
            Status::Archive => "Archive".to_owned(),
        }
    }
}

fn get_header(name: &str, headers: &[MessagePartHeader]) -> Option<String> {
    for h in headers {
        if h.name.is_none() {
            continue;
        }
        if name.eq_ignore_ascii_case(&h.name.as_deref().unwrap()) {
            return h.value.clone();
        }
    }
    None
}

impl Email {
    fn new(message: &gmail1::api::Message) -> Option<Self> {
        let headers = &message.payload.as_ref()?.headers.as_deref()?;
        let subject = get_header("subject", headers).unwrap_or_default();
        let from_raw = get_header("from", headers).unwrap_or_default();
        let from_vec: Vec<_> = from_raw.split(",").collect_vec();
        let from = if from_vec.is_empty() || from_vec.len() > 1 {
            from_raw
            //get_header("sender", headers).unwrap_or_default()
        } else {
            from_vec.first().unwrap().deref().to_owned()
        };
        let pos = from.find("<").unwrap_or(0usize);
        let from = &from[pos..];
        let from_parts = from.split("@").collect_vec();
        let from_name = from_parts.get(0).unwrap_or(&"").deref().to_owned();
        let from_domain = from_parts.get(1).unwrap_or(&"").deref().to_owned();
        let date: i64 = message.internal_date.as_ref()?.parse().ok()?;
        Some(Email {
            status: Status::Inbox,
            subject,
            body: message.snippet.as_deref()?.to_owned(),
            from_name,
            from_domain,
            date,
            id: message.id.as_deref()?.to_owned(),
        })
    }
}

async fn fetch_inbox_ids<F: Fn(usize, &str)>(gmail: &Gmail, f: &F) -> Option<Vec<String>> {
    let mut ids: Vec<String> = vec![];
    let mut page_token: Option<String> = None;
    loop {
        let query = gmail
            .users()
            .messages_list("me")
            .add_label_ids("INBOX")
            .add_scope(String::from("https://mail.google.com/"));
        let query = if page_token.is_none() {
            query
        } else {
            query.page_token(&page_token.unwrap())
        };
        let response = query.doit().await;
        if response.is_err() {
            break;
        }
        let response = response.unwrap();
        page_token = response.1.next_page_token.clone();
        let current_ids = response.1.messages;
        if current_ids.is_none() {
            break;
        }
        let current_ids = current_ids.unwrap();
        for m in current_ids {
            if m.id.is_none() {
                continue;
            }
            ids.push(m.id.unwrap());
        }
        f(ids.len(), &format!("{:?}", &ids));
        if page_token.is_none() {
            break;
        }
        //        break;
    }

    Some(ids)
}

fn update_status(current: &mut Email, status: &Option<Status>) {
    if status.is_some() {
        current.status = status.clone().unwrap();
    }
}

async fn fetch_inbox<F: Fn(usize, &str)>(gmail: &Gmail, f: F) -> Vec<Email> {
    let mut emails: Vec<Email> = vec![];
    let ids = fetch_inbox_ids(gmail, &f).await.unwrap_or_default();

    for id in &ids {
        let result = gmail
            .users()
            .messages_get("me", id)
            .format("full")
            .add_scope(String::from("https://mail.google.com/"))
            .doit()
            .await;
        f(emails.len(), "Getting");
        f(emails.len(), &format!("{:?}", &result));
        if result.is_err() {
            f(emails.len(), "");
            continue;
        }
        let email = Email::new(&result.unwrap().1);
        if email.is_none() {
            continue;
        }
        emails.push(email.unwrap());
    }
    emails
}

async fn move_chunks<F: Fn(&str, u32, u32)>(
    gmail: &Gmail,
    inbox: &str,
    to: &Vec<String>,
    label: &str,
    f: &F,
) -> anyhow::Result<()> {
    let mut i: u32 = 032;
    for chunk in &to.iter().chunks(100) {
        let v: Vec<_> = chunk.map(|i| i.clone()).collect();
        let chunk_size =  v.len() as u32;
        let mut request = BatchModifyMessagesRequest::default();
        request.ids = Some(v);
        if label != "archive" {
            request.add_label_ids = Some(vec![label.to_owned()]);
        }
        request.remove_label_ids = Some(vec![inbox.to_owned()]);
        gmail
            .users()
            .messages_batch_modify(request, "me")
            .add_scope(String::from("https://mail.google.com/"))
            .doit()
            .await?;
        f(label, chunk_size, to.len() as u32);
        i += 1;
    }
    Ok(())
}

async fn move_emails<F: Fn(&str, u32, u32)>(
    gmail: &Gmail,
    emails: &Vec<Email>,
    _inbox: &str,
    archive: &str,
    follow_up: &str,
    read_through: &str,
    f: F,
) -> anyhow::Result<()> {
    let to_archive: Vec<_> = emails
        .iter()
        .filter(|e| e.status == Status::Archive)
        .map(|e| e.id.clone())
        .collect();
    let to_follow_up: Vec<_> = emails
        .iter()
        .filter(|e| e.status == Status::FollowUp)
        .map(|e| e.id.clone())
        .collect();
    let to_read_through: Vec<_> = emails
        .iter()
        .filter(|e| e.status == Status::ReadThrough)
        .map(|e| e.id.clone())
        .collect();

    move_chunks(&gmail, "INBOX", &to_follow_up, follow_up, &f).await?;
    move_chunks(&gmail, "INBOX", &to_follow_up, follow_up, &f).await?;
    move_chunks(&gmail, "INBOX", &to_read_through, read_through, &f).await?;
    move_chunks(&gmail, "INBOX", &to_archive, archive, &f).await?;

    Ok(())
}

#[tokio::main]
async fn main() {
    // Get an ApplicationSecret instance by some means. It contains the `client_id` and
    // `client_secret`, among other things.

    let secret = oauth2::read_application_secret("./client_secret.json")
        .await
        .expect("client_secret.json");

    // Create an authenticator that uses an InstalledFlow to authenticate. The
    // authentication tokens are persisted to a file named tokencache.json. The
    // authenticator takes care of caching tokens to disk and refreshing tokens once
    // they've expired.
    let auth = oauth2::InstalledFlowAuthenticator::builder(
        secret,
        oauth2::InstalledFlowReturnMethod::HTTPRedirect,
    )
    .persist_tokens_to_disk("./tokens")
    .build()
    .await
    .unwrap();

    let hub = Gmail::new(
        hyper::Client::builder().build(hyper_rustls::HttpsConnector::with_native_roots()),
        auth,
    );

    let labels = hub.users().labels_list("me").doit().await.unwrap().1;
    let labels = labels.labels.unwrap();
    let mut label_map = HashMap::<String, String>::default();
    for label in labels {
        label_map.insert(label.name.clone().unwrap(), label.id.clone().unwrap());
        println!("{:?}", &label.name.unwrap());
    }
    // return;

    /* dotenv::dotenv().unwrap();

    let password = dotenv::var("IMAP_PASSWORD").ok().unwrap_or(String::new());
    let user = dotenv::var("IMAP_USER").ok().unwrap_or(String::new());
    let domain = dotenv::var("IMAP_DOMAIN").ok().unwrap_or(String::new());
    let follow_up = dotenv::var("IMAP_FOLLOWUP").ok().unwrap();
    let read_through = dotenv::var("IMAP_READTHROUGH").ok().unwrap();
    let archive = dotenv::var("IMAP_ARCHIVE").ok().unwrap();

    */
    let follow_up = dotenv::var("IMAP_FOLLOWUP")
        .ok()
        .unwrap_or("follow-up".to_owned());
    let read_through = dotenv::var("IMAP_READTHROUGH")
        .ok()
        .unwrap_or("read-through".to_owned());
    let archive = dotenv::var("IMAP_ARCHIVE")
        .ok()
        .unwrap_or("archive".to_owned());

    let follow_up = label_map.get(&follow_up).unwrap().clone();
    let read_through = label_map.get(&read_through).unwrap().clone();

    let window = pancurses::initscr();
    let mut emails = fetch_inbox(&hub, |c, _| {
        window.clear();
        window.addstr(format!("Read {} emails", c));
        window.refresh();
    })
    .await;
    emails.sort_by(|a, b| {
        (&a.from_domain, &a.from_name, &a.date)
            .cmp(&(&b.from_domain, &b.from_name, &b.date))
            .reverse()
    });
    let mut i = 0usize;
    if emails.is_empty() {
        pancurses::endwin();
        println!("No emails");
        return;
    }
    let mut needs_refresh = true;
    let mut change_status: Option<Status> = None;
    loop {
        if i >= emails.len() {
            i = emails.len() - 1;
        }
        if needs_refresh {
            let text = format!(
                "Status: {} of {} {}\n\nDate:{} \n\nFrom:{}@{}\n\nSubject:{}\n\n{}",
                i + 1,
                emails.len(),
                emails[i].status.to_string(),
                chrono::NaiveDateTime::from_timestamp(emails[i].date / 1_000, 0),
                emails[i].from_name,
                emails[i].from_domain,
                emails[i].subject,
                emails[i].body
            );
            window.clear();
            window.addstr(text);
            let mut cy = window.get_cur_y();
            let y = window.get_max_y();
            while cy < y {
                window.addstr("\n");
                cy += 1;
            }
            window.mv(y, 0);
            window.refresh();
            window.clrtoeol();
            window.refresh();
            needs_refresh = false;
        }
        match window.getch().unwrap() {
            Character('q') => {
                pancurses::endwin();
                return;
            }
            Character('a') => change_status = Some(Status::Archive),
            Character('f') => change_status = Some(Status::FollowUp),
            Character('r') => change_status = Some(Status::ReadThrough),
            Character('i') => change_status = Some(Status::Inbox),

            Character('0') => {
                i = 0;
                needs_refresh = true;
            }
            Character('t') => {
                for pos in i..emails.len(){
                    if emails[pos].subject == emails[i].subject {
                        update_status(&mut emails[pos], &change_status);
                    }
                }
                change_status = None;
                needs_refresh = true;
            }
            Character('[') => {
                let bracketed_current = get_bracketed(&emails[i].subject).trim().to_owned();
                if(!bracketed_current.is_empty()){
                    for pos in i..emails.len(){
                        let bracketed_pos = get_bracketed(&emails[pos].subject).trim().to_owned();
                        if bracketed_current == bracketed_pos {
                            update_status(&mut emails[pos], &change_status);
                        }
                    }
               }
               change_status = None;
               needs_refresh = true;
            }


            Character('j') => {
                update_status(&mut emails[i], &mut change_status);
                if i < emails.len() - 1 {
                    i += 1;
                }
                change_status = None;
                needs_refresh = true;
            }
            Character('k') => {
                update_status(&mut emails[i], &mut change_status);
                if i > 0 {
                    i -= 1;
                }
                change_status = None;
                needs_refresh = true;
            }
            Character('p') => {
                update_status(&mut emails[i], &mut change_status);
                let mut j = i;
                while j < emails.len() - 1
                    && emails[j].from_name == emails[i].from_name
                    && emails[j].from_domain == emails[i].from_domain
                {
                    update_status(&mut emails[j], &mut change_status);
                    j += 1;
                }
                i = j;
                change_status = None;
                needs_refresh = true;
            }
            Character('d') => {
                update_status(&mut emails[i], &mut change_status);
                let mut j = i;
                while j < emails.len() - 1 && emails[j].from_domain == emails[i].from_domain {
                    update_status(&mut emails[j], &mut change_status);
                    j += 1;
                }
                i = j;
                change_status = None;
                needs_refresh = true;
            }
            Character('w') => {
                move_emails(
                    &hub,
                    &emails,
                    "INBOX",
                    &archive,
                    &follow_up,
                    &read_through,
                    |s, count, total| {
                        window.clear();
                        window.addstr(format!("Moved {} of {} to {}", count, total, s));
                        window.refresh();
                    },
                )
                .await
                .unwrap();

                let v: Vec<_> = emails
                    .into_iter()
                    .filter(|e| e.status == Status::Inbox)
                    .collect();
                emails = v;
                if emails.is_empty() {
                    window.clear();
                    window.addstr(format!("No emails"));
                    window.refresh();
                    return;
                }
                i = 0;
            }
            pancurses::Input::KeyEnter => needs_refresh = true,

            _ => (),
        }
    }
}
