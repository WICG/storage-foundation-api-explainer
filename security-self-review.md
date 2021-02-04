# Storage Foundation API Security Self-Review

https://www.w3.org/TR/2020/NOTE-security-privacy-questionnaire-20200508/

## 2.1. What information might this feature expose to Web sites or other parties, and for what purposes is that exposure necessary?

Since this feature is a storage API, it does not expose any information that is
not first created by the origin.

## 2.2. Is this specification exposing the minimum amount of information necessary to power the feature?

Yes, we only expose data that was persisted by the origin.

## 2.3. How does this specification deal with personal information or personally-identifiable information or information derived thereof?

This specification does not expose any personal information.

## 2.4. How does this specification deal with sensitive information?

This specification does not deal with sensitive information.

## 2.5. Does this specification introduce new state for an origin that persists across browsing sessions?

Yes, this specification introduces a new persistent storage mechanism where
state may be kept. The risk of tracking must be mitigated by following similar
strategies used in other storage APIs e.g. user agents should allow users to
wipe stored data, an origin may only access files that are owned by it, access
in third-party contexts should be governed by the same policy as other storage
types, etc.

## 2.6. What information from the underlying platform, e.g. configuration data, is exposed by this specification to an origin?

No information from the underlying platform is exposed. However care must be
taken during implementation, such as not to expose operating-system-specific
behavior through the way file operations are executed.

## 2.7. Does this specification allow an origin access to sensors on a user’s device

This specification does not allow access to sensors.

## 2.8. What data does this specification expose to an origin? Please also document what data is identical to data exposed by other features, in the same or different contexts.

The data exposed is only the one that was previously created by the origin. This
is similar to other storage APIs

## 2.9. Does this specification enable new script execution/loading mechanisms?

This specification does not enable new script execution/loading mechanisms.

## 2.10. Does this specification allow an origin to access other devices?

This specification does not allow access to other devices.

## 2.11. Does this specification allow an origin some measure of control over a user agent’s native UI?

This specification does not allow any control of a user agent’s UI.

## 2.12. What temporary identifiers might this specification create or expose to the web?

This specification does not expose any temporary identifiers.

## 2.13. How does this specification distinguish between behavior in first-party and third-party contexts?

This specification follows the same user-agent policies that apply to
other storage types. It’s worth noting that the data accessible to each context
will depend on the origin, and therefore a call from a third-party context would
not be able to access first-party content.

## 2.14. How does this specification work in the context of a user agent’s Private Browsing or "incognito" mode?

This specification will work mostly the same as in “regular” mode, except that
stored information will not be persisted after a private session is finished.
This  should be indistinguishable from other storage APIs.

## 2.15. Does this specification have a "Security Considerations" and "Privacy Considerations" section?

Yes.

## 2.16. Does this specification allow downgrading default security characteristics?

This specification does not allow downgrading any security characteristics.

## 2.17. What should this questionnaire have asked?

One of the privacy risks we’ve thought about comes from the way we assume most
implementations would expose file operations: we were worried about OS-specific
file errors or -patterns being observable through our API. This questionnaire
could consider asking if there are any risks that should be especially mitigated
in a “natural” or “common approach” implementation.
